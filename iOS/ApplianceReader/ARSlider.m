//
//  ARSlider.m
//  ApplianceReader
//

#import "ARSlider.h"


#pragma mark - Implementation

@implementation ARSlider

- (void)accessibilityDecrement
{
    self.value -= 0.1;
    [self sendActionsForControlEvents:UIControlEventValueChanged];
}

- (void)accessibilityIncrement
{
    self.value += 0.1;
    [self sendActionsForControlEvents:UIControlEventValueChanged];
}

- (NSString *)accessibilityValue
{
    return [NSString stringWithFormat:@"%.1f", self.value];
}

@end
