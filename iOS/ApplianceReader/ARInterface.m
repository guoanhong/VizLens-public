//
//  ARInterface.m
//  ApplianceReader
//

#import "ARInterface.h"


#pragma mark - Implementation

@implementation ARInterface

- (NSString *)description
{
    return [NSString stringWithFormat:@"<Interface Name: %@, Status: %@>", self.name, self.status];
}

// Use designated initializer.
- (instancetype)init
{
    return [self initWithName:nil status:nil];
}

- (instancetype)initWithName:(NSString *)name status:(NSString *)status
{
    self = [super init];
    if (self)
    {
        _name = [name copy];
        _status = [status copy];
    }
    return self;
}

+ (instancetype)interfaceWithName:(NSString *)name status:(NSString *)status
{
    NSParameterAssert(name);
    NSParameterAssert(status);
    return [[self alloc] initWithName:name status:status];
}

- (BOOL)isEqual:(id)object
{
    if (self == object)
    {
        return YES;
    }
    if (![object isKindOfClass:[ARInterface class]])
    {
        return NO;
    }
    return [self isEqualToInterface:(ARInterface *)object];
}

- (BOOL)isEqualToInterface:(ARInterface *)interface
{
    if (!interface)
    {
        return NO;
    }
    BOOL isEqualName = [self.name isEqualToString:interface.name];
    BOOL isEqualStatus = [self.status isEqualToString:interface.status];
    return (isEqualName && isEqualStatus);
}

- (NSUInteger)hash
{
    return [self.name hash] ^ [self.status hash];
}

@end
