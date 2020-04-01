//
//  ARTarget.h
//  ApplianceReader
//

#import <Foundation/Foundation.h>
#import <CoreGraphics/CGGeometry.h>


#pragma mark - Interface

@interface ARTarget : NSObject

@property (nonatomic, copy) NSString *name;
@property (nonatomic) CGRect normalizedFrame;

// Convenience.
+ (instancetype)targetWithTargetArray:(NSArray *)targetArray;

@end
