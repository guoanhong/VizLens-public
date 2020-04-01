//
//  ARCollectionViewInterfaceLayout.h
//  ApplianceReader
//

#import <UIKit/UIKit.h>


#pragma mark - Collection View Interface Layout Data Source

@protocol ARCollectionViewInterfaceLayoutDataSource <NSObject>

@required

- (CGSize)normalizedContentSizeForCollectionView:(nullable UICollectionView *)collectionView;
- (nullable NSArray *)targetsForCollectionView:(nullable UICollectionView *)collectionView;

@end


#pragma mark - Interface

@interface ARCollectionViewInterfaceLayout : UICollectionViewLayout

@property (nonatomic, weak, nullable) id <ARCollectionViewInterfaceLayoutDataSource> interfaceLayoutDataSource;

@end
