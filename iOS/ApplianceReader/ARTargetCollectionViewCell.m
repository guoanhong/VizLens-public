//
//  ARTargetCollectionViewCell.m
//  ApplianceReader
//

#import "ARTargetCollectionViewCell.h"

#import "ARAppDelegate.h"


#pragma mark - Helpers

NS_INLINE CGColorRef borderColorForSelection(BOOL selection)
{
    return ( selection ) ? yellowColor().CGColor : blueColor().CGColor;
}


#pragma mark - Implementation

@implementation ARTargetCollectionViewCell

- (void)layoutSubviews
{
    [super layoutSubviews];
    
    CALayer *layer = self.contentView.layer;
    layer.borderColor = borderColorForSelection(self.isSelected);
    layer.borderWidth = 1.0f;
    layer.cornerRadius = 4.0f;
    layer.masksToBounds = YES;
}

- (void)setSelected:(BOOL)selected
{
    [super setSelected:selected];
    
    self.contentView.layer.borderColor = borderColorForSelection(selected);
    self.textLabel.textColor = ( selected ) ? yellowColor() : blueColor();
}

@end
