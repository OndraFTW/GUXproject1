/*
    Author: Ondřej Šlampa, xslamp01@stud.fit.vutbr.cz
    Project: GUX proj1
*/

/*
    Standard XToolkit and OSF/Motif include files.
*/
#include <X11/Intrinsic.h>
#include <Xm/Xm.h> 

/*
    Public include files for widgets used in this file.
*/
#include <Xm/MainW.h> 
#include <Xm/Form.h> 
#include <Xm/Frame.h>
#include <Xm/DrawingA.h>
#include <Xm/PushB.h>
#include <Xm/RowColumn.h>
#include <Xm/ComboBox.h>
#include <Xm/ToggleB.h>
#include <Xm/MessageB.h>
#include <Xm/Label.h>
#include <Xm/Protocols.h>
#include <X11/Xmu/Editres.h>

/*
    Common C library include files
*/
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

/*
    Shared variables
*/

GC drawGC=0;			/* GC used for final drawing */
GC inputGC=0;			/* GC used for drawing current position */

int x1, y1, x2, y2;		/* input points */ 
int button_pressed=0;	/* input state */

//type of drawable shape
typedef enum shape_type{
    POINT=0,LINE,RECTANGLE,FILLED_RECTANGLE,ELIPSE,FILLED_ELIPSE
}
ShapeType;

//currently selected shape
ShapeType selected_shape=POINT;

//type of line width
typedef enum width_type{
    NONE=0,THIN,NORMAL,THICK
}
WidthType;

//currently selected line width
WidthType selected_width=THIN;

//type of color
typedef enum color_type{
    WHITE=0,RED,GREEN,BLUE,BLACK
}
ColorType;

//currently selected foreground and background color
ColorType selected_line_front=BLACK;
ColorType selected_line_back=WHITE;
ColorType selected_fill_front=BLACK;
ColorType selected_fill_back=WHITE;

//data about drawable shape
typedef struct shape{
    int x1,y1,x2,y2;
    ShapeType type;
    WidthType width;
    bool dash;
    ColorType line_front,line_back,fill_front,fill_back;
}
Shape;

//is line dashed
bool selected_dash=false;

//array of drawed shapes
Shape* shapes=NULL;

#define SHAPES_ALLOC_STEP	10
int maxshapes=0;
int nshapes=0;

//list of names of shapes
#define NUMBER_OF_SHAPES	6
char* shape_list[NUMBER_OF_SHAPES]={"Point","Line","Rectangle","Filled Rectangle","Elipse","Filled Elipse"};

//list of names of widths
#define NUMBER_OF_WIDTHS	4
char* width_list[NUMBER_OF_WIDTHS]={"None","Thin","Normal","Thick"};

//list of names of colors
#define NUMBER_OF_COLORS	5
char* color_list[NUMBER_OF_COLORS]={"White","Red","Green","Blue","Black"};

//array of colors
XColor xcolors[NUMBER_OF_COLORS];

//canvasfor drawing
Widget drawArea;
//quit dialog window
Widget question;

/*
    Draws a shape.
*/
void DrawShape(Display* d, Drawable w, GC gc, ShapeType type, WidthType width,
    bool dash, ColorType line_front, ColorType line_back,
    ColorType fill_front,ColorType fill_back, int x1, int y1, int x2, int y2){

    //get line width
    unsigned int line_width=1;
    switch(width){
        case NONE:line_width=0;break;
        case THIN:line_width=1;break;
        case NORMAL:line_width=3;break;
        case THICK:line_width=8;break;
    }
    
    //get line style
    int line_style=dash?LineDoubleDash:LineSolid;
    
    //set line attributes
    XSetLineAttributes(d, gc, line_width, line_style, CapNotLast, JoinMiter);
    
    //draw shape
    switch(type){
	    case POINT:{
	        //set colors
            XSetForeground(d,gc,xcolors[line_front].pixel);
            XSetBackground(d,gc,xcolors[line_back].pixel);
	        switch(width){
	            case NONE:XDrawPoint(d, w, gc, x1, y1);break;
	            case THIN:XFillArc(d, w, gc, x1-1, y1-1, 3, 3, 0, 360*64);break;
	            case NORMAL:XFillArc(d, w, gc, x1-2, y1-2, 5, 5, 0, 360*64);break;
	            case THICK:XFillArc(d, w, gc, x1-4, y1-4, 8, 8, 0, 360*64);break;
	        }
	    }
	    break;
	    case LINE:{
	        //set colors
            XSetForeground(d,gc,xcolors[line_front].pixel);
            XSetBackground(d,gc,xcolors[line_back].pixel);
	        XDrawLine(d, w, gc, x1, y1, x2, y2);
	    }
	    break;
	    case RECTANGLE:{
	        //set colors
            XSetForeground(d,gc,xcolors[line_front].pixel);
            XSetBackground(d,gc,xcolors[line_back].pixel);
	        if(x1<x2){
	            if(y1<y2){
	                XDrawRectangle(d, w, gc, x1, y1, x2-x1, y2-y1);
	            }
	            else{
	                XDrawRectangle(d, w, gc, x1, y2, x2-x1, y1-y2);
	            }
	        }
	        else{
	            if(y1<y2){
	                XDrawRectangle(d, w, gc, x2, y1, x1-x2, y2-y1);
	            }
	            else{
	                XDrawRectangle(d, w, gc, x2, y2, x1-x2, y1-y2);
	            }
	        }
	    }
	    break;
	    case FILLED_RECTANGLE:{
	        int cx,cy,sx,sy;
	        if(x1<x2){
	            if(y1<y2){
	                cx=x1;cy=y1;sx=x2-x1;sy=y2-y1;
	            }
	            else{
	                cx=x1;cy=y2;sx=x2-x1;sy=y1-y2;
	            }
	        }
	        else{
	            if(y1<y2){
	                cx=x2;cy=y1;sx=x1-x2;sy=y2-y1;
	            }
	            else{
	                cx=x2;cy=y2;sx=x1-x2;sy=y1-y2;
	            }
	        }
            XSetForeground(d,gc,xcolors[fill_front].pixel);
            XSetBackground(d,gc,xcolors[fill_back].pixel);
	        XFillRectangle(d, w, gc, cx, cy, sx, sy);
            XSetForeground(d,gc,xcolors[line_front].pixel);
            XSetBackground(d,gc,xcolors[line_back].pixel);
            XDrawRectangle(d, w, gc, cx, cy, sx, sy);
	    }
	    break;
	    case ELIPSE:{
	        //set colors
            XSetForeground(d,gc,xcolors[line_front].pixel);
            XSetBackground(d,gc,xcolors[line_back].pixel);
	        if(x1<x2){
	            if(y1<y2){
	                XDrawArc(d, w, gc, x1-(x2-x1), y1-(y2-y1),(x2-x1)*2,(y2-y1)*2, 0, 360*64);
	            }
	            else{
	                XDrawArc(d, w, gc, x1-(x2-x1), y1-(y1-y2),(x2-x1)*2,(y1-y2)*2, 0, 360*64);
	            }
	        }
	        else{
	            if(y1<y2){
	                XDrawArc(d, w, gc, x1-(x1-x2), y1-(y2-y1),(x1-x2)*2,(y2-y1)*2, 0, 360*64);
	            }
	            else{
	                XDrawArc(d, w, gc, x1-(x1-x2), y1-(y1-y2),(x1-x2)*2,(y1-y2)*2, 0, 360*64);
	            }
	        }
	    }
	    break;
	    case FILLED_ELIPSE:{
	        int cx,cy,sx,sy;
	        if(x1<x2){
	            if(y1<y2){
	                cx=x1-(x2-x1);cy=y1-(y2-y1);sx=(x2-x1)*2;sy=(y2-y1)*2;
	            }
	            else{
	                cx=x1-(x2-x1);cy=y1-(y1-y2);sx=(x2-x1)*2;sy=(y1-y2)*2;
	            }
	        }
	        else{
	            if(y1<y2){
	                cx=x1-(x1-x2);cy=y1-(y2-y1);sx=(x1-x2)*2;sy=(y2-y1)*2;
	            }
	            else{
	                cx=x1-(x1-x2);cy=y1-(y1-y2);sx=(x1-x2)*2;sy=(y1-y2)*2;
	            }
	        }
	        XSetForeground(d,gc,xcolors[fill_front].pixel);
            XSetBackground(d,gc,xcolors[fill_back].pixel);
	        XFillArc(d, w, gc, cx, cy, sx, sy, 0, 360*64);
	        XSetForeground(d,gc,xcolors[line_front].pixel);
            XSetBackground(d,gc,xcolors[line_back].pixel);
            XDrawArc(d, w, gc, cx, cy, sx, sy, 0, 360*64);
	    }
	    break;
	    default:{
	        printf("DrawShape: Uknown shape %d\n",selected_shape);
	    }
    }
}

/*
    Callback for dragging shape.
*/
void InputLineEH(Widget w, XtPointer client_data, XEvent *event, Boolean *cont){
    Pixel fg, bg;

    if(button_pressed){
	    if(!inputGC){
	        inputGC=XCreateGC(XtDisplay(w), XtWindow(w), 0, NULL);
	        XSetFunction(XtDisplay(w), inputGC, GXxor);
	        XSetPlaneMask(XtDisplay(w), inputGC, ~0);
	        XtVaGetValues(w, XmNforeground, &fg, XmNbackground, &bg, NULL);
	        XSetForeground(XtDisplay(w), inputGC, fg ^ bg);
	    }

	    if(button_pressed>1){
	        /* erase previous position */
	        DrawShape(XtDisplay(w), XtWindow(w), inputGC, selected_shape,
	            selected_width, selected_dash, selected_line_front,
	            selected_line_back, selected_fill_front, selected_fill_back,
	            x1, y1, x2, y2);
	    }
	    else{
	        /* remember first MotionNotify */
	        button_pressed=2;
	    }

	    x2=event->xmotion.x;
	    y2=event->xmotion.y;
	    
	    DrawShape(XtDisplay(w), XtWindow(w), inputGC, selected_shape,
	        selected_width, selected_dash, selected_line_front,
	        selected_line_back, selected_fill_front, selected_fill_back,
	        x1, y1, x2, y2);
    }
}

/*
    Callback for drawing new shape.
*/
void DrawLineCB(Widget w, XtPointer client_data, XtPointer call_data){
    Arg al[4];
    int ac;
    XGCValues v;
    XmDrawingAreaCallbackStruct *d=(XmDrawingAreaCallbackStruct*) call_data;

    switch(d->event->type){
	    //button pressed set first point of shape
	    case ButtonPress:
	        if(d->event->xbutton.button==Button1){
		        button_pressed=1;
		        x1=d->event->xbutton.x;
		        y1=d->event->xbutton.y;
	        }
	    break;
	    //button released - set second point of shape, save it and draw it
	    case ButtonRelease:
	        if(d->event->xbutton.button==Button1){
		        if(++nshapes > maxshapes){
		            maxshapes += SHAPES_ALLOC_STEP;
		            shapes=(Shape*) XtRealloc((char*)shapes,
		               (Cardinal)(sizeof(Shape)*maxshapes));
		        }

		        shapes[nshapes-1].x1=x1;
		        shapes[nshapes-1].y1=y1;
		        shapes[nshapes-1].x2=x2=d->event->xbutton.x;
		        shapes[nshapes-1].y2=y2=d->event->xbutton.y;
		        shapes[nshapes-1].type=selected_shape;
		        shapes[nshapes-1].width=selected_width;
		        shapes[nshapes-1].dash=selected_dash;
		        shapes[nshapes-1].line_front=selected_line_front;
		        shapes[nshapes-1].line_back=selected_line_back;
		        shapes[nshapes-1].fill_front=selected_fill_front;
		        shapes[nshapes-1].fill_back=selected_fill_back;

		        button_pressed=0;

		        if(!drawGC){
		            ac=0;
		            XtSetArg(al[ac], XmNforeground, &v.foreground); ac++;
		            XtGetValues(w, al, ac);
		            drawGC=XCreateGC(XtDisplay(w), XtWindow(w),
			        GCForeground, &v);
		        }
		        
		        DrawShape(XtDisplay(w), XtWindow(w), drawGC, selected_shape,
		            selected_width, selected_dash, selected_line_front,
	                selected_line_back, selected_fill_front, selected_fill_back,
		            x1, y1, x2, y2);
	        }
	    break;
    }
}

/*
    Callback for exposing canvas.
*/
void ExposeCB(Widget w, XtPointer client_data, XtPointer call_data){

    if(nshapes<=0){
	    return;
	}
    if(!drawGC){
	    drawGC=XCreateGC(XtDisplay(w), XtWindow(w), 0, NULL);
    }
    for(int i=0;i<nshapes;i++){
        DrawShape(XtDisplay(w), XtWindow(w), drawGC, shapes[i].type,
            shapes[i].width, shapes[i].dash, shapes[i].line_front,
            shapes[i].line_back, shapes[i].fill_front, shapes[i].fill_back,
            shapes[i].x1, shapes[i].y1, shapes[i].x2, shapes[i].y2);
    }
}

/*
    Callback for clearing.
*/
void ClearCB(Widget w, XtPointer client_data, XtPointer call_data){
    nshapes=0;
    XClearWindow(XtDisplay(drawArea), XtWindow(drawArea));
}

/*
    Callback for quit dialog.
*/
void QuitCB(Widget w, XtPointer client_data, XtPointer call_data){
    XtManageChild(question);
}

/*
    Callback for selecting shape.
*/
void SelectShapeCB(Widget w, XtPointer client_data, XtPointer call_data){ 
    XmComboBoxCallbackStruct *data=(XmComboBoxCallbackStruct *) call_data;
    selected_shape=data->item_position;
}

/*
    Callback for selecting line width.
*/
void SelectWidthCB(Widget w, XtPointer client_data, XtPointer call_data){ 
    XmComboBoxCallbackStruct *data=(XmComboBoxCallbackStruct *) call_data;
    selected_width=data->item_position;
}

/*
    Callback for seting line style.
*/
void SelectDashCB(Widget w, XtPointer client_data, XtPointer call_data){ 
    XmToggleButtonCallbackStruct *data=(XmToggleButtonCallbackStruct *) call_data;
    selected_dash=data->set;
}

/*
    Callback for selecting foreground color.
*/
void SelectLineFrontCB(Widget w, XtPointer client_data, XtPointer call_data)
{ 
    XmComboBoxCallbackStruct *data=(XmComboBoxCallbackStruct *) call_data;
    selected_line_front=data->item_position;
}

/*
    Callback for selecting background color.
*/
void SelectLineBackCB(Widget w, XtPointer client_data, XtPointer call_data){ 
    XmComboBoxCallbackStruct *data=(XmComboBoxCallbackStruct *) call_data;
    selected_line_back=data->item_position;
}

/*
    Callback for selecting foreground color.
*/
void SelectFillFrontCB(Widget w, XtPointer client_data, XtPointer call_data)
{ 
    XmComboBoxCallbackStruct *data=(XmComboBoxCallbackStruct *) call_data;
    selected_fill_front=data->item_position;
}

/*
    Callback for selecting background color.
*/
void SelectFillBackCB(Widget w, XtPointer client_data, XtPointer call_data){ 
    XmComboBoxCallbackStruct *data=(XmComboBoxCallbackStruct *) call_data;
    selected_fill_back=data->item_position;
}

/*
    Callback for file menu.
*/
void FileCB(Widget widget, XtPointer clientData, XtPointer callData){
	if ((size_t)clientData==0){
		ClearCB(widget, clientData, callData);
	}
	else if ((size_t)clientData==1){
		QuitCB(widget, clientData, callData);
    }
}

/*
    Callback for quit dialog.
*/
void QuestionCB(Widget w, XtPointer client_data, XtPointer call_data){
    switch ((size_t)client_data){
        case 0: /* ok */
            exit(0);
        break;
        case 1: /* cancel */
        break;
    }
}

/*
    Main function.
*/
int main(int argc, char **argv){
    XtAppContext app_context;
    Widget topLevel, mainWin, menuBar, frame, rowColumn, quitBtn, clearBtn,
        shapeComboBox, widthComboBox, dashCheckBox,lineFrontComboBox,
        lineBackComboBox, fillFrontComboBox, fillBackComboBox;

    /*
     * Register the default language procedure
     */
    XtSetLanguageProc(NULL,(XtLanguageProc)NULL, NULL);

    topLevel=XtVaAppInitialize(
      &app_context,		 	/* Application context */
      "Draw",				/* Application class */
      NULL, 0,				/* command line option list */
      &argc, argv,			/* command line args */
      NULL,				/* for missing app-defaults file */
      XmNdeleteResponse, XmDO_NOTHING,
      NULL);				/* terminate varargs list */

    mainWin=XtVaCreateManagedWidget(
      "mainWin",			/* widget name */
      xmMainWindowWidgetClass,		/* widget class */
      topLevel,				/* parent widget*/
      XmNcommandWindowLocation, XmCOMMAND_BELOW_WORKSPACE,
      XmNwidth, 800,
	  XmNheight, 450,
      NULL);				/* terminate varargs list */

    menuBar=XmVaCreateSimpleMenuBar(
	  mainWin,
	  "menuBar",
	  XmVaCASCADEBUTTON, XmStringCreateSimple("File"), XK_F,
	  NULL);

	XmVaCreateSimplePulldownMenu(
	  menuBar,
	  "fileMenu",
	  0,
	  FileCB,
      XmVaPUSHBUTTON, XmStringCreateSimple("Clear"), XK_C, "Alt<Key>C", XmStringCreateSimple("Alt+C"),
	  XmVaPUSHBUTTON, XmStringCreateSimple("Quit"),  XK_Q, "Alt<Key>Q", XmStringCreateSimple("Alt+Q"),
	  NULL);
    
    XtManageChild(menuBar);
    
    frame=XtVaCreateManagedWidget(
      "frame",				/* widget name */
      xmFrameWidgetClass,		/* widget class */
      mainWin,				/* parent widget */
      NULL);				/* terminate varargs list */
    
    rowColumn=XtVaCreateManagedWidget(
      "rowColumn",			/* widget name */
      xmRowColumnWidgetClass,		/* widget class */
      frame,				/* parent widget */
      XmNentryAlignment, XmALIGNMENT_CENTER,	/* alignment */
      XmNorientation, XmVERTICAL,	/* orientation */
      XmNpacking, XmPACK_TIGHT,	/* packing mode */
      NULL);				/* terminate varargs list */
    
    clearBtn=XtVaCreateManagedWidget(
      "Clear",				/* widget name */
      xmPushButtonWidgetClass,		/* widget class */
      rowColumn,			/* parent widget*/
      NULL);				/* terminate varargs list */

    quitBtn=XtVaCreateManagedWidget(
      "Quit",				/* widget name */
      xmPushButtonWidgetClass,		/* widget class */
      rowColumn,			/* parent widget*/
      NULL);				/* terminate varargs list */

    XtVaCreateManagedWidget(
      "ShapeLabel",
      xmLabelWidgetClass,
      rowColumn,
      XmNlabelString, XmStringCreateSimple("Shape:"),
      NULL);

    XmStringTable str_list=(XmStringTable) XtMalloc(NUMBER_OF_SHAPES*sizeof(XmString));

    for(int i=0; i < NUMBER_OF_SHAPES; i++)
        str_list[i]=XmStringCreateLocalized(shape_list[i]);

    shapeComboBox=XtVaCreateManagedWidget(
      "ShapeSelector",
      xmComboBoxWidgetClass,
      rowColumn,
      XmNcomboBoxType, XmDROP_DOWN_LIST,
      XmNitemCount, NUMBER_OF_SHAPES,
      XmNitems, str_list,
      XmNpositionMode, XmZERO_BASED,
      NULL);
    
    XtVaCreateManagedWidget(
      "WidthLabel",
      xmLabelWidgetClass,
      rowColumn,
      XmNlabelString, XmStringCreateSimple("Line width:"),
      NULL);
    
    str_list=(XmStringTable) XtMalloc(NUMBER_OF_WIDTHS*sizeof(XmString));

    for(int i=0; i < NUMBER_OF_WIDTHS; i++)
        str_list[i]=XmStringCreateLocalized(width_list[i]);
    
    widthComboBox=XtVaCreateManagedWidget(
      "WidthSelector",
      xmComboBoxWidgetClass,
      rowColumn,
      XmNcomboBoxType, XmDROP_DOWN_LIST,
      XmNitemCount, NUMBER_OF_WIDTHS,
      XmNitems, str_list,
      XmNselectedPosition, selected_width,
      XmNpositionMode, XmZERO_BASED,
      NULL);
    
    dashCheckBox=XtVaCreateManagedWidget(
      "Dash line",
      xmToggleButtonWidgetClass,
      rowColumn,
      XmNindicatorType, XmN_OF_MANY,
      NULL);

    str_list=(XmStringTable) XtMalloc(NUMBER_OF_COLORS * sizeof(XmString));

    for(int i=0; i < NUMBER_OF_COLORS; i++)
        str_list[i]=XmStringCreateLocalized(color_list[i]);
    
    XtVaCreateManagedWidget(
      "LineFrontLabel",
      xmLabelWidgetClass,
      rowColumn,
      XmNlabelString, XmStringCreateSimple("Line foreground color:"),
      NULL);
    
    lineFrontComboBox=XtVaCreateManagedWidget(
      "LineFrontSelector",
      xmComboBoxWidgetClass,
      rowColumn,
      XmNcomboBoxType, XmDROP_DOWN_LIST,
      XmNitemCount, NUMBER_OF_COLORS,
      XmNitems, str_list,
      XmNselectedPosition, selected_line_front,
      XmNpositionMode, XmZERO_BASED,
      NULL);
    
    XtVaCreateManagedWidget(
      "LineBackLabel",
      xmLabelWidgetClass,
      rowColumn,
      XmNlabelString, XmStringCreateSimple("Line background color:"),
      NULL);
    
    lineBackComboBox=XtVaCreateManagedWidget(
      "LineBackSelector",
      xmComboBoxWidgetClass,
      rowColumn,
      XmNcomboBoxType, XmDROP_DOWN_LIST,
      XmNitemCount, NUMBER_OF_COLORS,
      XmNitems, str_list,
      XmNselectedPosition, selected_line_back,
      XmNpositionMode, XmZERO_BASED,
      NULL);

    XtVaCreateManagedWidget(
      "FillFrontLabel",
      xmLabelWidgetClass,
      rowColumn,
      XmNlabelString, XmStringCreateSimple("Fill foreground color:"),
      NULL);
    
    fillFrontComboBox=XtVaCreateManagedWidget(
      "FillFrontSelector",
      xmComboBoxWidgetClass,
      rowColumn,
      XmNcomboBoxType, XmDROP_DOWN_LIST,
      XmNitemCount, NUMBER_OF_COLORS,
      XmNitems, str_list,
      XmNselectedPosition, selected_fill_front,
      XmNpositionMode, XmZERO_BASED,
      NULL);
    
    XtVaCreateManagedWidget(
      "FillBackLabel",
      xmLabelWidgetClass,
      rowColumn,
      XmNlabelString, XmStringCreateSimple("Fill background color:"),
      NULL);
    
    fillBackComboBox=XtVaCreateManagedWidget(
      "FillBackSelector",
      xmComboBoxWidgetClass,
      rowColumn,
      XmNcomboBoxType, XmDROP_DOWN_LIST,
      XmNitemCount, NUMBER_OF_COLORS,
      XmNitems, str_list,
      XmNselectedPosition, selected_fill_back,
      XmNpositionMode, XmZERO_BASED,
      NULL);

    Display* display=XOpenDisplay(NULL);
    int screen=DefaultScreen(display);
    Colormap colormap=DefaultColormap(display,screen);

    xcolors[0].red=65535;xcolors[0].green=65535;xcolors[0].blue=65535;
    xcolors[1].red=65535;xcolors[1].green=0;xcolors[1].blue=0;
    xcolors[2].red=0;xcolors[2].green=65535;xcolors[2].blue=0;
    xcolors[3].red=0;xcolors[3].green=0;xcolors[3].blue=65535;
    xcolors[4].red=0;xcolors[4].green=0;xcolors[4].blue=0;
    
    for(int i=0;i<NUMBER_OF_COLORS;i++){
        xcolors[i].flags=DoRed|DoGreen|DoBlue;
        XAllocColor(display,colormap,&xcolors[i]);
    }
    
    drawArea=XtVaCreateManagedWidget(
      "drawingArea",			/* widget name */
      xmDrawingAreaWidgetClass,		/* widget class */
      frame,				/* parent widget*/
      XmNbackground, xcolors[WHITE].pixel,
      NULL);				/* terminate varargs list */
    
/*
    XSelectInput(XtDisplay(drawArea), XtWindow(drawArea), 
      KeyPressMask | KeyReleaseMask | ButtonPressMask | ButtonReleaseMask
      | Button1MotionMask );
*/
    question=XmCreateQuestionDialog(topLevel, "Question", NULL, 0);
    XtVaSetValues(
     question,
     XmNdialogStyle, XmDIALOG_FULL_APPLICATION_MODAL,
     XmNmessageString, XmStringCreateSimple("Do you realy want to quit?"),
     XmNokLabelString, XmStringCreateSimple("OK"),
     XmNcancelLabelString, XmStringCreateSimple("Cancel"),
     NULL);
    XtUnmanageChild(XmMessageBoxGetChild(question, XmDIALOG_HELP_BUTTON));
    XtAddCallback(question, XmNokCallback, QuestionCB, (XtPointer)0);
    XtAddCallback(question, XmNcancelCallback, QuestionCB, (XtPointer)1);

    XtAddCallback(drawArea, XmNinputCallback, DrawLineCB, drawArea);
    XtAddEventHandler(drawArea, ButtonMotionMask, False, InputLineEH, NULL);
    XtAddCallback(drawArea, XmNexposeCallback, ExposeCB, drawArea);

    XtAddCallback(clearBtn, XmNactivateCallback, ClearCB, 0);
    XtAddCallback(quitBtn, XmNactivateCallback, QuitCB, 0);
    XtAddCallback(shapeComboBox, XmNselectionCallback, SelectShapeCB, 0);
    XtAddCallback(widthComboBox, XmNselectionCallback, SelectWidthCB, 0);
    XtAddCallback(dashCheckBox, XmNvalueChangedCallback, SelectDashCB, 0);
    XtAddCallback(lineFrontComboBox, XmNselectionCallback, SelectLineFrontCB, 0);
    XtAddCallback(lineBackComboBox, XmNselectionCallback, SelectLineBackCB, 0);
    XtAddCallback(fillFrontComboBox, XmNselectionCallback, SelectFillFrontCB, 0);
    XtAddCallback(fillBackComboBox, XmNselectionCallback, SelectFillBackCB, 0);

    XtRealizeWidget(topLevel);
    Atom wm_delete=XInternAtom(XtDisplay(topLevel), "WM_DELETE_WINDOW", false);
    XmAddWMProtocolCallback(topLevel, wm_delete, QuitCB, NULL);
    XmActivateWMProtocol(topLevel, wm_delete);
    XtAddEventHandler(topLevel, 0, true, _XEditResCheckMessages, NULL);
    XtAppMainLoop(app_context);

    return 0;
}

