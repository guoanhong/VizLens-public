//
//  ARTarget.m
//  ApplianceReader
//

#import "ARTarget.h"


#pragma mark - Implementation

@implementation ARTarget

+ (instancetype)targetWithTargetArray:(NSArray *)targetArray
{
    ARTarget *target = [[self alloc] init];
    target.name = (NSString *)targetArray[0];
    target.normalizedFrame = CGRectMake(((NSString *)targetArray[1]).floatValue,
                                        ((NSString *)targetArray[2]).floatValue,
                                        ((NSString *)targetArray[3]).floatValue - ((NSString *)targetArray[1]).floatValue,
                                        ((NSString *)targetArray[4]).floatValue - ((NSString *)targetArray[2]).floatValue);
    return target;
}

- (NSString *)description
{
    CGRect normalizedFrame = self.normalizedFrame;
    return [NSString stringWithFormat:@"<%@: x:%.3f, y:%.3f, width:%.3f, height:%.3f>",
            self.name,
            CGRectGetMinX(normalizedFrame),
            CGRectGetMinY(normalizedFrame),
            CGRectGetWidth(normalizedFrame),
            CGRectGetHeight(normalizedFrame)];
}

- (NSUInteger)hash
{
    return [self.name hash];
}

- (BOOL)isEqual:(id)object
{
    if (self == object)
    {
        return YES;
    }
    if (![object isKindOfClass:[ARTarget class]])
    {
        return NO;
    }
    return [self isEqualToTarget:(ARTarget *)object];
}

- (BOOL)isEqualToTarget:(ARTarget *)target
{
    if (!target)
    {
        return NO;
    }
    BOOL isEqualName = [self.name isEqualToString:target.name];
    BOOL isEqualNormalizedFrame = CGRectEqualToRect(self.normalizedFrame, target.normalizedFrame);
    return (isEqualName && isEqualNormalizedFrame);
}

@end
