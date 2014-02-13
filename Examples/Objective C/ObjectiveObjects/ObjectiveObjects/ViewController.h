//
//  ViewController.h
//  ObjectiveObjects
//
//  Created by User on 8/9/13.
//  Copyright (c) 2013 XML. All rights reserved.
//

#import <UIKit/UIKit.h>

@interface ViewController : UIViewController

// User Interface Bindings
@property (nonatomic, strong) IBOutlet UIButton * button1;
@property (nonatomic, strong) IBOutlet UIButton * button2;
@property (nonatomic, strong) IBOutlet UIButton * button3;
@property (nonatomic, strong) IBOutlet UITextView * textView;

// Public Methods

- (IBAction)test1:(id)sender;
- (IBAction)test2:(id)sender;
- (IBAction)test3:(id)sender;

@end
