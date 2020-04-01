//
//  ARInterface.h
//  ApplianceReader
//

#import <Foundation/Foundation.h>


#pragma mark - Interface

@interface ARInterface : NSObject

@property (nonatomic, copy) NSString *name;
@property (nonatomic, copy) NSString *status;

// Designated initializer.
- (instancetype)initWithName:(NSString *)name status:(NSString *)status;

// Convenience.
+ (instancetype)interfaceWithName:(NSString *)name status:(NSString *)status;

@end
