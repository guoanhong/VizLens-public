//
//  ARInterfaceListTableViewController.m
//  ApplianceReader
//

#import "ARInterfaceListTableViewController.h"

#import "ARAppDelegate.h"
#import "ARInterface.h"
#import "ARInterfaceFeedbackViewController.h"
#import "ARSettingsViewController.h"


#pragma mark - Globals

static NSString *ARInterfaceTableViewCellReuseIdentifier = @"ARInterfaceTableViewCellReuseIdentifier";
static NSString *AROptionTableViewCellReuseIdentifier = @"AROptionTableViewCellReuseIdentifier";


#pragma mark - Helpers

NS_INLINE NSString *accessibilityLabelForCell(UITableViewCell *cell)
{
    NSMutableString *accessibilityLabel = nil;
    if (cell)
    {
        [accessibilityLabel appendString:cell.textLabel.text];
        if (cell.textLabel.text.length)
        {
            [accessibilityLabel appendFormat:@", %@", cell.detailTextLabel.text];
        }
    }
    return [accessibilityLabel copy];
}

NS_INLINE BOOL isFirstIndexPathRow(NSIndexPath *indexPath)
{
    return indexPath && (indexPath.row == 0);
}

NS_INLINE BOOL isLastIndexPathRowForTableView(NSIndexPath *indexPath, UITableView *tableView)
{
    return indexPath && tableView && (indexPath.row == ([tableView numberOfRowsInSection:0] - 1));
}

NS_INLINE BOOL isProcessingStatus(ARInterface *interface)
{
    return [interface.status isEqualToString:@"processing"];
}

NS_INLINE BOOL isReadyStatus(ARInterface *interface)
{
    return [interface.status isEqualToString:@"ready"];
}

NS_INLINE UIColor *interfaceStatusColorForInterface(ARInterface *interface)
{
    if (isReadyStatus(interface))
    {
        return greenColor();
    }
    else if (isProcessingStatus(interface))
    {
        return yellowColor();
    }
    return redColor();
}


#pragma mark - Private Interface

@interface ARInterfaceListTableViewController () <UITableViewDataSource, UITableViewDelegate>

// Other.
@property (nonatomic, readonly) ARInterfaceFeedbackViewController *interfaceFeedbackViewController;
@property (nonatomic) NSArray *interfaceListArray;
@property (nonatomic, readonly) ARSettingsViewController *settingsViewController;

// Queue.
@property (nonatomic) BOOL isGETingInterfaces;
@property (nonatomic, readonly) dispatch_source_t refreshInterfaceListDispatchSource;

@end


#pragma mark - Implementation

@implementation ARInterfaceListTableViewController

- (BOOL)prefersStatusBarHidden
{
    return YES;
}

- (void)viewDidDisappear:(BOOL)animated
{
    [super viewDidDisappear:animated];
    
    dispatch_suspend(self.refreshInterfaceListDispatchSource);
}

- (void)viewDidLoad
{
    [super viewDidLoad];
    
    [self.tableView registerNib:[UINib nibWithNibName:@"ARInterfaceTableViewCell" bundle:nil]
         forCellReuseIdentifier:ARInterfaceTableViewCellReuseIdentifier];
    [self.tableView registerNib:[UINib nibWithNibName:@"AROptionTableViewCell" bundle:nil]
         forCellReuseIdentifier:AROptionTableViewCellReuseIdentifier];
}

- (void)viewWillAppear:(BOOL)animated
{
    [super viewWillAppear:animated];
    
    self.isGETingInterfaces = NO;
    
    dispatch_resume(self.refreshInterfaceListDispatchSource);
}


#pragma mark - Table View Data Source

- (NSInteger)numberOfSectionsInTableView:(UITableView *)tableView
{
    return 1;
}

- (NSInteger)tableView:(UITableView *)tableView numberOfRowsInSection:(NSInteger)section
{
    // 'Add Appliance', 'Settings', and any interfaces that have been added.
    return (2 + self.interfaceListArray.count);
}

- (UITableViewCell *)tableView:(UITableView *)tableView cellForRowAtIndexPath:(NSIndexPath *)indexPath
{
    UITableViewCell *cell = nil;
    
    // Configure 'Add Appliance' table view cell.
    if (isFirstIndexPathRow(indexPath))
    {
        cell = [tableView dequeueReusableCellWithIdentifier:AROptionTableViewCellReuseIdentifier
                                               forIndexPath:indexPath];
        cell.textLabel.text = @"Add Interface";
    }
    // Configure 'Settings' table view cell.
    else if (isLastIndexPathRowForTableView(indexPath, tableView))
    {
        cell = [tableView dequeueReusableCellWithIdentifier:AROptionTableViewCellReuseIdentifier
                                               forIndexPath:indexPath];
        cell.textLabel.text = @"Settings";
    }
    // Configure interface table view cells.
    else
    {
        cell = [tableView dequeueReusableCellWithIdentifier:ARInterfaceTableViewCellReuseIdentifier
                                               forIndexPath:indexPath];
        ARInterface *interface = (ARInterface *)self.interfaceListArray[MAX(indexPath.row, 1) - 1];
        NSString *interfaceStatus = interface.status;
        cell.detailTextLabel.text = interfaceStatus;
        cell.detailTextLabel.textColor = interfaceStatusColorForInterface(interface);
        cell.textLabel.text = interface.name;
        // Keep 'processing' status targets from being selected.
        cell.userInteractionEnabled = !isProcessingStatus(interface);
    }
    
    // Accessibility.
    cell.accessibilityLabel = accessibilityLabelForCell(cell);
    
    // Set selection color.
    UIView *selectedBackgroundView = [[UIView alloc] init];
    selectedBackgroundView.backgroundColor = selectionColor();
    cell.selectedBackgroundView = selectedBackgroundView;
    
    return cell;
}


#pragma mark - Table View Delegate

- (void)tableView:(UITableView *)tableView didSelectRowAtIndexPath:(NSIndexPath *)indexPath
{
    // Handle 'Add Appliance' table view cell selection.
    if (isFirstIndexPathRow(indexPath))
    {
        ARInterfaceFeedbackViewController *interfaceFeedbackViewController = self.interfaceFeedbackViewController;
        interfaceFeedbackViewController.mode = ARInterfaceFeedbackViewControllerCaptureMode;
        [self presentViewController:interfaceFeedbackViewController animated:YES completion:NULL];
    }
    // Handle 'Settings' table view cell selection.
    else if (isLastIndexPathRowForTableView(indexPath, tableView))
    {
        [self presentViewController:self.settingsViewController animated:YES completion:NULL];
    }
    // Handle interface table view cell selection.
    else
    {
        ARInterfaceFeedbackViewController *interfaceFeedbackViewController = self.interfaceFeedbackViewController;
        ARInterface *interface = (ARInterface *)self.interfaceListArray[MAX(indexPath.row, 1) - 1];
        interfaceFeedbackViewController.interface = interface;
        if (isReadyStatus(interface))
        {
            interfaceFeedbackViewController.mode = ARInterfaceFeedbackViewControllerFeedbackMode;
        }
        else
        {
            interfaceFeedbackViewController.mode = ARInterfaceFeedbackViewControllerRecaptureMode;
        }
        [self presentViewController:interfaceFeedbackViewController animated:YES completion:NULL];
    }
}


#pragma mark - Other

@synthesize interfaceFeedbackViewController = _interfaceFeedbackViewController;

- (ARInterfaceFeedbackViewController *)interfaceFeedbackViewController
{
    if (!_interfaceFeedbackViewController)
    {
        _interfaceFeedbackViewController = [[ARInterfaceFeedbackViewController alloc] initWithNibName:@"ARInterfaceFeedbackViewController"
                                                                                               bundle:nil];
    }
    return _interfaceFeedbackViewController;
}

@synthesize settingsViewController = _settingsViewController;

- (ARSettingsViewController *)settingsViewController
{
    if (!_settingsViewController)
    {
        _settingsViewController = [[ARSettingsViewController alloc] initWithNibName:@"ARSettingsViewController"
                                                                             bundle:nil];
    }
    return _settingsViewController;
}

- (NSArray *)interfaceListArrayForInterfaceListString:(NSString *)interfaceListString
{
    NSMutableArray *interfaceListArray = [NSMutableArray array];
    
    for (NSString *interfaceString in [interfaceListString componentsSeparatedByString:@";"])
    {
        NSArray *interfaceStringComponents = [interfaceString componentsSeparatedByString:@"|"];
        // Ignore interfaces that lack a name or a status (e.g. the 'add'
        // that is appended to the end of every interface list string).
        if (interfaceStringComponents.count == 2)
        {
            ARInterface *interface = [ARInterface interfaceWithName:interfaceStringComponents[0]
                                                             status:interfaceStringComponents[1]];
            [interfaceListArray addObject:interface];
        }
    }
    
    return [interfaceListArray copy];
}

@synthesize refreshInterfaceListDispatchSource = _refreshInterfaceListDispatchSource;

- (dispatch_source_t)refreshInterfaceListDispatchSource
{
    if (!_refreshInterfaceListDispatchSource)
    {
        dispatch_source_t refreshInterfaceListDispatchSource = dispatch_source_create(DISPATCH_SOURCE_TYPE_TIMER, 0, 0, dispatch_get_main_queue());
        ARInterfaceListTableViewController * __weak weakSelf = self;
        dispatch_source_set_event_handler(refreshInterfaceListDispatchSource, ^{
            if (!weakSelf.isGETingInterfaces)
            {
                weakSelf.isGETingInterfaces = YES;
                
                ARAppDelegate *appDelegate = (ARAppDelegate *)[UIApplication sharedApplication].delegate;
                NSString *interfaceListURLString = [NSString stringWithFormat:@"http://%@/php/getObjectList.php?userid=%@", appDelegate.serverDomain, appDelegate.userID];
                NSURL *interfaceListURL = [NSURL URLWithString:interfaceListURLString];
                [[[NSURLSession sharedSession] dataTaskWithURL:interfaceListURL completionHandler:^(NSData * _Nullable data, NSURLResponse * _Nullable response, NSError * _Nullable error) {
                    NSArray *interfaceListArray = nil;
                    
                    // Create a new interface list array if possible.
                    if (data && isOKHTTPURLResponse(response))
                    {
                        NSString *interfaceListString = [[NSString alloc] initWithData:data encoding:NSUTF8StringEncoding];
                        interfaceListArray = [weakSelf interfaceListArrayForInterfaceListString:interfaceListString];
                    }
                    
                    // Update the user interface on the main thread.
                    dispatch_async(dispatch_get_main_queue(), ^{
                        if (interfaceListArray && ![interfaceListArray isEqualToArray:weakSelf.interfaceListArray])
                        {
                            weakSelf.interfaceListArray = interfaceListArray;
                            [weakSelf.tableView reloadSections:[NSIndexSet indexSetWithIndexesInRange:NSMakeRange(0, weakSelf.tableView.numberOfSections)]
                                              withRowAnimation:UITableViewRowAnimationAutomatic];
                        }
                    });
                    
                    weakSelf.isGETingInterfaces = NO;
                }] resume];
            }
        });
        dispatch_source_set_timer(refreshInterfaceListDispatchSource, DISPATCH_TIME_NOW, 1.0 * NSEC_PER_SEC, 0.1 * NSEC_PER_SEC);
        _refreshInterfaceListDispatchSource = refreshInterfaceListDispatchSource;
    }
    return _refreshInterfaceListDispatchSource;
}

@end
