/*******************************************************************************
	License
	****************************************************************************
	This program is free software; you can redistribute it
	and/or modify it under the terms of the GNU General
	Public License as published by the Free Software
	Foundation; either version 3 of the License, or
	(at your option) any later version.
 
	This program is distributed in the hope that it will
	be useful, but WITHOUT ANY WARRANTY; without even the
	implied warranty of MERCHANTABILITY or FITNESS FOR A
	PARTICULAR PURPOSE. See the GNU General Public
	License for more details.
 
	Licence can be viewed at
	http://www.gnu.org/licenses/gpl-3.0.txt

	Please maintain this license information along with authorship
	and copyright notices in any redistribution of this code
*******************************************************************************/
//
//  AVRMultiSketchTableViewController.h
//  AVRMultiSketch
//
//  Created by Jon on 11/2/18.
//  Copyright © 2018 Jon Mackey. All rights reserved.
//

#import <Cocoa/Cocoa.h>

NS_ASSUME_NONNULL_BEGIN

@interface AVRMultiSketchTableViewController : NSViewController <NSTableViewDataSource>
@property (weak) IBOutlet NSTableView *tableView;

@property(strong, nonatomic) NSMutableArray<NSMutableDictionary*>* sketches;
-(void)setData:(uint32_t)inStart length:(uint32_t)inLength forIndex:(NSUInteger)inIndex;
- (BOOL)setSketchesWithContentsOfURL:(NSURL*)inDocURL;
- (BOOL)writeSketchesToURL:(NSURL*)inDocURL;
@end

NS_ASSUME_NONNULL_END
