//
//  ARCollectionViewInterfaceLayout.m
//  ApplianceReader
//

#import "ARCollectionViewInterfaceLayout.h"

#import "ARTarget.h"


#pragma mark - Helpers

NS_INLINE UIEdgeInsets collectionViewContentEdgeInsets()
{
    return UIEdgeInsetsMake(8.0f, 80.0f, 8.0f, 80.0f);
}


#pragma mark - Implementation

@implementation ARCollectionViewInterfaceLayout

- (CGSize)collectionViewContentSize
{
    return self.collectionView.frame.size;
}

- (nullable NSArray<__kindof UICollectionViewLayoutAttributes *> *)layoutAttributesForElementsInRect:(CGRect)rect
{
    NSMutableArray *layoutAttributes = nil;
    
    NSArray *targets = [self targets];
    BOOL isTargets = (targets.count > 0);
    if (isTargets)
    {
        layoutAttributes = [NSMutableArray array];
        [targets enumerateObjectsUsingBlock:^(ARTarget *target, NSUInteger idx, BOOL * _Nonnull stop) {
            NSIndexPath *targetIndexPath = [NSIndexPath indexPathForRow:idx inSection:0];
            UICollectionViewLayoutAttributes *layoutAttributesForTargetAtIndexPath = [self layoutAttributesForItemAtIndexPath:targetIndexPath];
            if (CGRectIntersectsRect(layoutAttributesForTargetAtIndexPath.frame, rect))
            {
                [layoutAttributes addObject:layoutAttributesForTargetAtIndexPath];
            }
        }];
    }
    
    return [layoutAttributes copy];
}

- (UICollectionViewLayoutAttributes *)layoutAttributesForItemAtIndexPath:(NSIndexPath *)indexPath
{
    UICollectionViewLayoutAttributes *layoutAttributes = nil;
    
    NSArray *targets = [self targets];
    BOOL isTargetForIndexPath = (targets.count > indexPath.row);
    if (isTargetForIndexPath)
    {
        layoutAttributes = [UICollectionViewLayoutAttributes layoutAttributesForCellWithIndexPath:indexPath];
        CGRect normalizedTargetFrame = [(ARTarget *)targets[indexPath.row] normalizedFrame];
        CGFloat normalizingConstant = self.normalizingConstant;
        // Adjust all frames to center everything in the available space.
        CGRect rect = CGRectMake(CGRectGetMinX(normalizedTargetFrame) * normalizingConstant,
                                 CGRectGetMinY(normalizedTargetFrame) * normalizingConstant,
                                 CGRectGetWidth(normalizedTargetFrame) * normalizingConstant,
                                 CGRectGetHeight(normalizedTargetFrame) * normalizingConstant);
        CGSize collectionViewContentSize = [self contentSize];
        CGSize collectionViewFrameSize = self.collectionView.frame.size;
        CGFloat dx = (collectionViewFrameSize.width - collectionViewContentSize.width) / 2.0f;
        CGFloat dy = (collectionViewFrameSize.height - collectionViewContentSize.height) / 2.0f;
        layoutAttributes.frame = CGRectOffset(rect, dx, dy);
    }
    
    return layoutAttributes;
}


#pragma mark - Other

- (CGSize)contentSize
{
    CGSize normalizedContentSize = [self normalizedContentSize];
    CGFloat normalizingConstant = [self normalizingConstant];
    return CGSizeMake(normalizedContentSize.width * normalizingConstant,
                      normalizedContentSize.height * normalizingConstant);
}

- (CGSize)normalizedContentSize
{
    CGSize normalizedContentSize = CGSizeZero;
    
    id <ARCollectionViewInterfaceLayoutDataSource> interfaceLayoutDataSource = self.interfaceLayoutDataSource;
    if ( [interfaceLayoutDataSource respondsToSelector:@selector(normalizedContentSizeForCollectionView:)] )
    {
        normalizedContentSize = [interfaceLayoutDataSource normalizedContentSizeForCollectionView:self.collectionView];
    }
    
    return normalizedContentSize;
}

- (CGFloat)normalizingConstant
{
    CGSize normalizedContentSize = [self normalizedContentSize];
    CGFloat normalizedContentHeight = normalizedContentSize.height;
    CGFloat normalizedContentWidth = normalizedContentSize.width;
    CGSize collectionViewFrameSize = self.collectionView.frame.size;
    // Make room for edge insets.
    UIEdgeInsets edgeInsets = collectionViewContentEdgeInsets();
    CGFloat collectionViewFrameHeight = collectionViewFrameSize.height - (edgeInsets.bottom + edgeInsets.top);
    CGFloat collectionViewFrameWidth = collectionViewFrameSize.width - (edgeInsets.left + edgeInsets.right);
    return (normalizedContentHeight > normalizedContentWidth) ? collectionViewFrameHeight : collectionViewFrameWidth;
}

- (NSArray *)targets
{
    NSArray *targets = nil;
    
    id <ARCollectionViewInterfaceLayoutDataSource> interfaceLayoutDataSource = self.interfaceLayoutDataSource;
    if ( [interfaceLayoutDataSource respondsToSelector:@selector(targetsForCollectionView:)] )
    {
        targets = [interfaceLayoutDataSource targetsForCollectionView:self.collectionView];
    }
    
    return targets;
}

@end
