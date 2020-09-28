/* compiles with command line  g++ linkage.c -lX11 -lm  */
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xos.h>
#include <X11/Xatom.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include "Cell.h"
#include <stdbool.h>
#include <vector>
#include <algorithm>


Display *display_ptr;
Screen *screen_ptr;
int screen_num;
char *display_name = NULL;
unsigned int display_width, display_height;

Window win;
int border_width;
unsigned int win_width, win_height;
int win_x, win_y;

XWMHints *wm_hints;
XClassHint *class_hints;
XSizeHints *size_hints;
XTextProperty win_name, icon_name;
char *win_name_string = "Linkage Game";
char *icon_name_string = "Linkage";

XEvent report;

GC gc, gc_col1, gc_col2, gc_col3, gc_col4, gc_grey;
unsigned long valuemask = 0;
XGCValues gc_values, gc_col1_values, gc_col2_values,
gc_col3_values, gc_col4_values, gc_col5_values, gc_col6_values,
  gc_grey_values;
Colormap color_map;
XColor tmp_color1, tmp_color2;
/* global variables*/
int gameboard[7][7];
int colorpieces[5]; /*warning: I numbered colors 1 to 4, so colorpieces[0]
		      is meaningless*/
int fieldsize, offset;

Cell cell[7][7];
int group_counter = 0;
int selected_color, first_field_i, first_field_j, second_field_i, second_field_j;
vector<Slot> slot;

void redraw_board(); /* given below */
int game_ended(); /* needs to be written */
void determine_winner(); /* needs to be written */
void make_M_move(); /* needs to be written */
void make_L_move();  /* needs to be written */

int evaluate();
void find_near_ava_slot_L(int i, int j, int i_2, int j_2, vector<Slot> &slot);
void find_near_ava_slot_M(int i, int j, int i_2, int j_2, vector<Slot> &slot);
void visit_neighbour(Cell &cell);
int minimax(vector<Slot> slot, int depth, bool isMax);
vector<int> find_one_slot();
void detect_around_color(int i, int j, vector<int> &around_color);


int main(int argc, char **argv)
{ char computer_side;
  int i,j;
  int move_stage =0;
  if(argc != 2)
  { printf("Please call with argument M or L, to play as More or Less\n");
      exit(0);
  }
  else
  {  if( argv[1][0] =='M' )
     {  printf("Human plays More, computer plays Less\n");
        computer_side = 'L';
     }
     else if ( argv[1][0] =='L' )
     {  printf("Human plays Less, computer plays More\n");
        computer_side = 'M';
     }
     else
     {  printf("Please call with argument M or L, to play as More or Less\n");
        exit(0);
     }
  }   
  /* initialize gameboard and colorpieces */
  for(i=0; i<7; i++)
    for(j=0; j<7; j++)
      gameboard[i][j]=0; /*empty*/
  gameboard[3][3] = 7; /*blocked*/
  for( i=0; i<5; i++)
    colorpieces[i]=6; /* pieces for each color */
  
  /* opening display: basic connection to X Server */
  if( (display_ptr = XOpenDisplay(display_name)) == NULL )
    { printf("Could not open display. \n"); exit(-1);}
  printf("Connected to X server  %s\n", XDisplayName(display_name) );
  screen_num = DefaultScreen( display_ptr );
  screen_ptr = DefaultScreenOfDisplay( display_ptr );
  color_map  = XDefaultColormap( display_ptr, screen_num );
  display_width  = DisplayWidth( display_ptr, screen_num );
  display_height = DisplayHeight( display_ptr, screen_num );

  printf("Width %d, Height %d, Screen Number %d\n", 
           display_width, display_height, screen_num);
  /* creating the window */
  border_width = 10;
  win_x = 0; win_y = 0;
  win_height = (int) (display_height / 1.5); 
  win_width = (int)( 0.8* win_height);
  
  win= XCreateSimpleWindow( display_ptr, RootWindow( display_ptr, screen_num),
                            win_x, win_y, win_width, win_height, border_width,
                            BlackPixel(display_ptr, screen_num),
                            WhitePixel(display_ptr, screen_num) );
  /* now try to put it on screen, this needs cooperation of window manager */
  size_hints = XAllocSizeHints();
  wm_hints = XAllocWMHints();
  class_hints = XAllocClassHint();
  if( size_hints == NULL || wm_hints == NULL || class_hints == NULL )
    { printf("Error allocating memory for hints. \n"); exit(-1);}

  size_hints -> flags = PPosition | PSize | PMinSize  ;
  size_hints -> min_width = 60;
  size_hints -> min_height = 60;

  XStringListToTextProperty( &win_name_string,1,&win_name);
  XStringListToTextProperty( &icon_name_string,1,&icon_name);
  
  wm_hints -> flags = StateHint | InputHint ;
  wm_hints -> initial_state = NormalState;
  wm_hints -> input = False;

  class_hints -> res_name = "linkage_game_interface";
  class_hints -> res_class = "examples";

  XSetWMProperties( display_ptr, win, &win_name, &icon_name, argv, argc,
                    size_hints, wm_hints, class_hints );

  /* what events do we want to receive */
  XSelectInput( display_ptr, win, 
            ExposureMask | StructureNotifyMask | ButtonPressMask );
  
  /* finally: put window on screen */
  XMapWindow( display_ptr, win );

  XFlush(display_ptr);

  /* create graphics context, so that we may draw in this window */
  gc = XCreateGC( display_ptr, win, valuemask, &gc_values);
  XSetForeground( display_ptr, gc, BlackPixel( display_ptr, screen_num ) );
  XSetLineAttributes( display_ptr, gc, 4, LineSolid, CapRound, JoinRound);
  /* and seven other graphics contexts, for the game colors and grey*/
  /* color1: orange */
  gc_col1 = XCreateGC( display_ptr, win, valuemask, &gc_col1_values);
  XSetLineAttributes(display_ptr, gc_col1, 6, LineSolid, CapRound, JoinRound);
  if( XAllocNamedColor( display_ptr, color_map, "orange", 
			&tmp_color1, &tmp_color2 ) == 0 )
    {printf("failed to get color 1 orange\n"); exit(-1);} 
  else
    XSetForeground( display_ptr, gc_col1, tmp_color1.pixel );
  /* color2: purple */
  gc_col2 = XCreateGC( display_ptr, win, valuemask, &gc_col2_values);
  XSetLineAttributes(display_ptr, gc_col2, 6, LineSolid, CapRound, JoinRound);
  if( XAllocNamedColor( display_ptr, color_map, "purple", 
			&tmp_color1, &tmp_color2 ) == 0 )
    {printf("failed to get color 2 purple\n"); exit(-1);} 
  else
    XSetForeground( display_ptr, gc_col2, tmp_color1.pixel );
  /* color3: green*/
  gc_col3 = XCreateGC( display_ptr, win, valuemask, &gc_col3_values);
  XSetLineAttributes(display_ptr, gc_col3, 6, LineSolid, CapRound, JoinRound);
  if( XAllocNamedColor( display_ptr, color_map, "green", 
			&tmp_color1, &tmp_color2 ) == 0 )
    {printf("failed to get color 3 green\n"); exit(-1);} 
  else
    XSetForeground( display_ptr, gc_col3, tmp_color1.pixel );
  /* color4: cyan */
  gc_col4 = XCreateGC( display_ptr, win, valuemask, &gc_col4_values);
  XSetLineAttributes(display_ptr, gc_col4, 6, LineSolid, CapRound, JoinRound);
  if( XAllocNamedColor( display_ptr, color_map, "cyan", 
			&tmp_color1, &tmp_color2 ) == 0 )
    {printf("failed to get color 4 cyan\n"); exit(-1);} 
  else
    XSetForeground( display_ptr, gc_col4, tmp_color1.pixel );
  /* color: grey */
  gc_grey = XCreateGC( display_ptr, win, valuemask, &gc_grey_values);
  if( XAllocNamedColor( display_ptr, color_map, "light grey", 
			&tmp_color1, &tmp_color2 ) == 0 )
    {printf("failed to get color grey\n"); exit(-1);} 
  else
    XSetForeground( display_ptr, gc_grey, tmp_color1.pixel );

  /* and now it starts: the event loop */
  while(1)
    { XNextEvent( display_ptr, &report );
      switch( report.type )
	{
	 case ConfigureNotify:
          /* This event happens when the user changes the size of the window*/
          win_width = report.xconfigure.width;
          win_height = report.xconfigure.height;
          redraw_board();
          break;
        case ButtonPress:
          /* This event happens when the user pushes a mouse button. */
	  {  /*int selected_color, first_field_i, first_field_j,
	         second_field_i, second_field_j;*/
             int x, y;
  	     x = report.xbutton.x;
             y = report.xbutton.y;
	     printf("processing  ButtonPress event; move_stage is %d\n", move_stage);
	     if( move_stage == 0 )
	     { /*should select color first*/
	       int color_select_failed = 0;
	       if( 2*offset+ 7*fieldsize < y && y < 2*offset + 8*fieldsize )
	       {  if( 2*offset < x && x < 2*offset + fieldsize )
		     selected_color = 1;
		   else if( 2*offset + fieldsize < x &&
			    x < 2*offset + 2*fieldsize )
		     selected_color = 2;
		   else if( 2*offset + 2*fieldsize < x &&
			    x < 2*offset + 3*fieldsize )
		     selected_color = 3;
		   else if( 2*offset + 3*fieldsize < x &&
			    x < 2*offset + 4*fieldsize )
		     selected_color = 4;
		   else 
                     color_select_failed = 1;
	       }
	       else
		  color_select_failed =1;
	       if( color_select_failed == 0 &&
		   colorpieces[selected_color] == 0)
	       {  color_select_failed = 1;
		  printf("no pieces of color %d left\n", selected_color);
               }
	       if( color_select_failed == 1)
	       {  printf("color selection failed, try again\n");
		  move_stage == 0;
	       }
	       else /* successfully selected color, now need to select field */
		  move_stage = 1;
		 
	     }
	     else if ( move_stage == 1)
	     { /*now select first field */
	       first_field_i = first_field_j = -1;
	       for( i=0; i<7; i++ )
	       {  for( j=0; j< 7; j++ )
		  {  if( offset + i*fieldsize < x &&
			 x < offset + (i+1)*fieldsize &&
			 offset + j*fieldsize < y &&
			 y < offset + (j+1)*fieldsize )
		      { first_field_i = i; first_field_j =j;}
		  }
	       }
	       if( first_field_i == -1 || first_field_j == -1
		   || gameboard[first_field_i][first_field_j] != 0 )
	       {  printf(" selection of first field failed; start over by color selection.\n");
		 move_stage =0;
	       }
	       else
		 move_stage = 2;

	     }
	     else if ( move_stage == 2)
	     { /* select second field, make move */
	       second_field_i = second_field_j =-1;
	       for( i=0; i<7; i++ )
	       {  for( j=0; j< 7; j++ )
		  {  if( offset + i*fieldsize < x &&
			 x < offset + (i+1)*fieldsize &&
			 offset + j*fieldsize < y &&
			 y < offset + (j+1)*fieldsize )
		      { second_field_i = i; second_field_j =j;}
		  }
	       }
	       if( second_field_i == -1 || second_field_j == -1 
		   || gameboard[second_field_i][second_field_j] != 0
		   || (   (second_field_i-first_field_i)
			  *(second_field_i-first_field_i)
			+ (second_field_j - first_field_j)
			  *(second_field_j - first_field_j) != 1 ) )
	       {
	        printf(" selection of second field failed; start over by color selection.\n");
		 move_stage =0;
	       }
	       else
	       {  /* we have a valid move, now put it on the gameboard */
		 gameboard[first_field_i][first_field_j] = selected_color;
		 gameboard[second_field_i][second_field_j] = selected_color;
		 colorpieces[selected_color] -= 1;
		 move_stage = 0;

		 redraw_board();
		 if( game_ended() )
		   determine_winner();
		 else
		 {  /* need computer move */
		 	if( computer_side == 'L' ){
		    make_L_move();
		 	/*for(int i=-1; i<8; i++){
		 		printf("(%d,-1).color: %d\n", i, gameboard[i][-1]);
		 		printf("(%d,7).color: %d\n", i, gameboard[i][7]);
		 	}

		 	for(int j=-1; j<8; j++){
		 		printf("(-1,%d).color: %d\n", j, gameboard[i][-1]);
		 		printf("(7,%d).color: %d\n", j, gameboard[i][7]);
		 	}*/
		   }

		   else
		   	make_M_move();
		   	redraw_board();

		   if( game_ended() )
		   	determine_winner();
		   /*game did not end, so need the next human move */
  		 }    
	       }
	     }
             else
	     {  printf(" invalid value %d in move_stage\n", move_stage);
		 exit(-1);
	     }
          }
          break;
          case Expose: /* redraw everything */
	    redraw_board();
          break;
	default:
	  /* this is a catch-all for other events; it does not do anything.
             One could look at the report type to see what the event was */ 
          break;
	}

    }
  exit(0);
}

void redraw_board()
{          int i,j;
           fieldsize = win_width / 8;
           offset = fieldsize / 2;

	  /* (re-)draw the gameboard. */

          {  for(i=0; i< 7; i++)
	     {  for(j=0; j<7; j++)
		{  switch( gameboard[i][j] )
		   {  case 0: /* empty field */
		       XFillRectangle(display_ptr, win, gc_grey,
				      offset+ i*fieldsize, offset+j*fieldsize,
				      fieldsize, fieldsize ); break;
		      case 1:
		       XFillRectangle(display_ptr, win, gc_col1,
				      offset+ i*fieldsize, offset+j*fieldsize,
				      fieldsize, fieldsize ); break;
		      case 2:
		       XFillRectangle(display_ptr, win, gc_col2,
				      offset+ i*fieldsize, offset+j*fieldsize,
				      fieldsize, fieldsize ); break;
		      case 3:
		       XFillRectangle(display_ptr, win, gc_col3,
				      offset+ i*fieldsize, offset+j*fieldsize,
				      fieldsize, fieldsize ); break;
		      case 4:
		       XFillRectangle(display_ptr, win, gc_col4,
				      offset+ i*fieldsize, offset+j*fieldsize,
				      fieldsize, fieldsize ); break;
		      case 7: /* blocked */
		       XDrawLine(display_ptr, win, gc,
				 offset+ i*fieldsize, offset+j*fieldsize,
				 offset+ (i+1)*fieldsize,
				 offset+(j+1)*fieldsize);
		       XDrawLine(display_ptr, win, gc,
				 offset+ (i+1)*fieldsize, offset+j*fieldsize,
				 offset+ i*fieldsize,
				 offset+(j+1)*fieldsize); break;
		     default: printf(" gameboard contained invalid entry %d at %d,%d\n", gameboard[i][j], i, j);
		       printf(" gameboard state:\n");
		     {  int tmpi, tmpj;
		        for( tmpi=0; tmpi< 7; tmpi++)
			{   printf("\n");
			    for(tmpj=0; tmpj<7; tmpj++)
			      printf("%2d ", gameboard[tmpi][tmpj]);
			}
                        printf("\n");
		     }
		     exit(-1); break;
		   }
		}
	     }
	     /* finished drawing colored fields for gameboard, 
                now draw outlines*/
	     /* vertical lines */
	     for(i=0; i<8; i++)
		     XDrawLine(display_ptr, win, gc,
			       offset+i*fieldsize, offset,
			       offset+i*fieldsize, offset + 7*fieldsize);
	     /* horizontal lines */
	     for(j=0; j<8; j++)
		     XDrawLine(display_ptr, win, gc,
			       offset, offset+j*fieldsize,
			       offset+7*fieldsize, offset + j*fieldsize);
	     /* now show the available colors */
	     XFillRectangle(display_ptr, win, gc_col1,
			     2*offset, 2*offset+7*fieldsize,
			     fieldsize, fieldsize );
	     XFillRectangle(display_ptr, win, gc_col2,
			     2*offset + fieldsize, 2*offset+7*fieldsize,
			     fieldsize, fieldsize );
	     XFillRectangle(display_ptr, win, gc_col3,
			     2*offset + 2*fieldsize, 2*offset+7*fieldsize,
			     fieldsize, fieldsize );
	     XFillRectangle(display_ptr, win, gc_col4,
			     2*offset + 3*fieldsize, 2*offset+7*fieldsize,
			     fieldsize, fieldsize );
	     for(i=0; i<4; i++)
	     {  XDrawRectangle(display_ptr, win, gc,
			     2*offset + i*fieldsize, 2*offset+7*fieldsize,
			     fieldsize, fieldsize );
		if( colorpieces[i+1] == 0)
		{ XDrawLine(display_ptr, win, gc,
			     2*offset + i*fieldsize,
			     2*offset + 7*fieldsize,
			     2*offset + (i+1)*fieldsize,
			     2*offset + 8*fieldsize );
		  XDrawLine(display_ptr, win, gc,
			     2*offset + (i+1)*fieldsize,
			     2*offset + 7*fieldsize,
			     2*offset + i*fieldsize,
			     2*offset + 8*fieldsize );
		}
	     }
	  }
}

/* terminal test */
int game_ended() {
	/* scan whole board, if 1 of 4 condition is true, return(0); else game end and return 1 */
	for(int i=0; i<7; i++){
		for(int j=0; j<7; j++){
			if(gameboard[i][j] == 0){
				if((i > 0) && (gameboard[i-1][j] == 0))
					return(0);
				if((i < 6) && (gameboard[i+1][j] == 0))
					return(0);
				if((j > 0) && (gameboard[i][j-1] == 0))
					return(0);
				if((j<6) && (gameboard[i][j+1] == 0))
					return(0);
			}
		}
	}
	return(1);
}

/* utility function */
void determine_winner() {
	/* cell reading */ 
	int i,j; /* i is column, j is raw */
	for(i=0; i<7; i++){
		for(j=0; j<7; j++){
			cell[i][j].color = gameboard[i][j];
			cell[i][j].i = i;
			cell[i][j].j = j;
			//printf("cell[%d][%d].visit %d \n", i,j,cell[i][j].color);
			if(cell[i][j].color == 0 || cell[i][j].color == 7){
				cell[i][j].visit = 1;
			}
			else
				cell[i][j].visit = 0;
			//printf("cell locate (%d,%d) %d \n", i, j, cell[i][j].visit);
		}
	}

	/* build neighbor structure */
	/* add neighbor to vector */
	for(i=0; i<7; i++){
		for(j=0; j<7; j++){
			if(i > 0){
				cell[i][j].neighbor.push_back(&cell[i-1][j]);
			}
			if(i < 6){
				cell[i][j].neighbor.push_back(&cell[i+1][j]);
			}
			if(j > 0){
				cell[i][j].neighbor.push_back(&cell[i][j-1]);
			}
			if(j < 6){
				cell[i][j].neighbor.push_back(&cell[i][j+1]);
			}	
		}
	}

	/* traverse the board */
	for(i=0; i<7; i++){
		for(j=0; j<7; j++){
			if(cell[i][j].visit != 1){
				visit_neighbour(cell[i][j]);
				group_counter++;
				printf("group score is %d.\n", group_counter);
			}
			
		}
	}

	printf("final group score is %d.\n", group_counter);

	if(group_counter < 12){
		printf("Less wins.\n"); 
	}
	else{
		printf("More wins.\n");
	}
	//printf("everyone wins.\n"); 

} /* exit(0); */


/* Max value */
void make_M_move() {
		/* human plays first */
		int depth = 6;
		int move = 0;
		int v = -100;
		vector<int> score;
	// find available slot around mouse click
	//printf("I am enter find_avaslot()\n");
		// find_near_ava_slot can imporve
		find_near_ava_slot_L(first_field_i,first_field_j,second_field_i,second_field_j, slot);
		/*int test_size = slot.size();
		for(int i=0; i<test_size; i++){
			printf("candidate slot coordinate list below\n");
			printf("i=%d, j=%d, i_2=%d, j_2=%d\n", slot[i].x, slot[i].y, slot[i].x_2, slot[i].y_2);
		}*/

	// draw color for cell
		cell[first_field_i][first_field_j].color = gameboard[first_field_i][first_field_j];
		cell[second_field_i][second_field_j].color = gameboard[second_field_i][second_field_j];

		int size = slot.size();
		if(size > 0){
			for(int i=0; i<size; i++){
				vector<Slot> temp;
				temp = slot;
			temp.erase(temp.begin()+i); // delete slot[i] from vector
			v = max(v, minimax(temp, depth,true));
			score.push_back(v);
			if(score[0] < v){
				score[0] = v;
				move = i;
			}

		}
	// get the candidate slot
		int x, y, x_2, y_2;
		x = slot[move].x;
		y = slot[move].y;
		x_2 = slot[move].x_2;
		y_2 = slot[move].y_2;

		printf("positions are: %d, %d, %d, %d \n", x, y, x_2, y_2);

	// get the color around the candidate slot
		vector<int> color;
		detect_around_color(x, y, color);
		detect_around_color(x_2, y_2, color); // get around color
	// print color
		int size = color.size();
		for(int i=0; i<size; i++){
			printf("around colors are: %d \n", color[i]);
		}	

		int color_count = 0;
		for(int index=1; index<5; index++){
			vector<int>::iterator it = find(color.begin(), color.end(), index);
			if(it != color.end()){
				color_count++;
				continue;
			}
			else{
				if(colorpieces[index] > 0){
					gameboard[x][y] = index;
					gameboard[x_2][y_2] = index;
					break;
				}
				else
					color_count++;
			}
			
		}

	// if not exist color different from around color, orderly select one color
		if(color_count == 4){
			int i = (rand() % (3)) + 1;
				if((selected_color != i) && (colorpieces[i] > 0)){
					gameboard[x][y] = i;
			  		gameboard[x_2][y_2] = i;
				}
			// for(int i=1; i<5; i++){
			// 	if(colorpieces[i] > 0){
			// 		gameboard[x][y] = i;
			// 		gameboard[x_2][y_2] = i;
			// 		break;
			// 	}
			// }
			printf("no color[i] is: %d \n", i);
		}


	// refresh the slot
		slot.clear();

	}
	// look for free slot
	else{
		vector<int> pos;
		pos = find_one_slot(); // get available position 1*2
		printf("candidate pos are: %d, %d, %d, %d\n", pos[0],pos[1],pos[2],pos[3]);
		vector<int> color;
		detect_around_color(pos[0], pos[1], color);
		detect_around_color(pos[2], pos[3], color); // get around color
		int size = color.size();
		for(int i=0; i<size; i++){
			printf("around colors are: %d\n", color[i]);
		}

		// traverse around color
		int color_count = 0;
		for(int index=4; index>0; index--){
			vector<int>::iterator it = find(color.begin(), color.end(), index);
			if(it != color.end()){
				color_count++;
				continue;
			}
			else{
				if(colorpieces[index] > 0){
					gameboard[pos[0]][pos[1]] = index;
					gameboard[pos[2]][pos[3]] = index;
					break;
				}
				else
					color_count++;
			}
	}

		// if not exist color different from around color, orderly select one color
				if(color_count == 4){
					for(int i=1; i<5; i++){
						if(colorpieces[i] > 0){
							gameboard[pos[0]][pos[1]] = i;
							gameboard[pos[2]][pos[3]] = i;
							break;
						}
					}
				}		
	}	
	//printf("I didn't make a decision\n");
	return;
} /* needs to be written */

/* Min value */
void make_L_move() {
	/* human plays first */
	int depth = 5;
	int move = 0;
	int v = 100;
	vector<int> score;
	// find available slot around mouse click
	//printf("I am enter find_avaslot()\n");
	find_near_ava_slot_L(first_field_i,first_field_j,second_field_i,second_field_j, slot);
	/*int test_size = slot.size();
	for(int i=0; i<test_size; i++){
		printf("candidate slot coordinate list below\n");
		printf("i=%d, j=%d, i_2=%d, j_2=%d\n", slot[i].x, slot[i].y, slot[i].x_2, slot[i].y_2);
	}*/


	// draw color for cell
	cell[first_field_i][first_field_j].color = gameboard[first_field_i][first_field_j];
	cell[second_field_i][second_field_j].color = gameboard[second_field_i][second_field_j];

	int size = slot.size();
	if(size > 0){
		for(int i=0; i<size; i++){
			vector<Slot> temp;
			temp = slot;
			temp.erase(temp.begin()+i); // delete slot[i] from vector
			v = min(v, minimax(temp, depth,false));
			score.push_back(v);
			if(score[0] > v){
				score[0] = v;
				move = i;
			}

		}
	// get the candidate slot
		int x, y, x_2, y_2;
		x = slot[move].x;
		y = slot[move].y;
		x_2 = slot[move].x_2;
		y_2 = slot[move].y_2;

	// draw color
		gameboard[x][y] = selected_color;
		gameboard[x_2][y_2] = selected_color;

	// refresh the slot
		slot.clear();

	}
	// if there is no available slot, look for free slot in the board
	else{
		vector<int> pos;
		pos = find_one_slot(); // get available position 1*2
		printf("candidate pos are: %d, %d, %d, %d\n", pos[0],pos[1],pos[2],pos[3]);
		vector<int> color;
		detect_around_color(pos[0], pos[1], color);
		detect_around_color(pos[2], pos[3], color); // get around color

		// traverse around color
		int size = color.size();
		// if have one color around
		if(size > 0){
			for(int i=0; i<size; i++){
			if((color[i] == 1) && (colorpieces[1] > 0)){
				gameboard[pos[0]][pos[1]] = 1;
				gameboard[pos[2]][pos[3]] = 1;
				break;
			}
			else if((color[i] == 2) && (colorpieces[2] > 0)){
				gameboard[pos[0]][pos[1]] = 2;
				gameboard[pos[2]][pos[3]] = 2;
				break;
			}
			else if((color[i] == 3) && (colorpieces[3] > 0)){
				gameboard[pos[0]][pos[1]] = 3;
				gameboard[pos[2]][pos[3]] = 3;
				break;
			}
			else if((color[i] == 4) && (colorpieces[4] > 0)){
				gameboard[pos[0]][pos[1]] = 4;
				gameboard[pos[2]][pos[3]] = 4;
				break;
			}
		}

		}
		// if no color around
		else{
			for(int i=1; i<5; i++){
				if(colorpieces[i] > 0){
					gameboard[pos[0]][pos[1]] = i;
					gameboard[pos[2]][pos[3]] = i;
					break;
				}
			}
		}
}
		
	//printf("I didn't make a decision\n");
	return;
}  /* needs to be written */


/* evaluation function: calculate socre */
int evaluate() {
	int i,j; /* i is column, j is raw */
	for(i=0; i<7; i++){
		for(j=0; j<7; j++){
			/*cell[i][j].color = gameboard[i][j];
			cell[i][j].i = i;
			cell[i][j].j = j;*/
			if(cell[i][j].color == 0 || cell[i][j].color == 7){
				cell[i][j].visit = 1;
			}
			else
				cell[i][j].visit = 0;
		}
	}

	/* build neighbor structure */
	/* add neighbor to vector */
	for(i=0; i<7; i++){
		for(j=0; j<7; j++){
			if(i > 0){
				cell[i][j].neighbor.push_back(&cell[i-1][j]);
			}
			if(i < 6){
				cell[i][j].neighbor.push_back(&cell[i+1][j]);
			}
			if(j > 0){
				cell[i][j].neighbor.push_back(&cell[i][j-1]);
			}
			if(j < 6){
				cell[i][j].neighbor.push_back(&cell[i][j+1]);
			}	
		}
	}

	/* traverse the board */
	int current_score = 0;
	for(i=0; i<7; i++){
		for(j=0; j<7; j++){
			if(cell[i][j].visit != 1){
				visit_neighbour(cell[i][j]);
				current_score++;
			}
			
		}
	}
	// // refresh cell[][]
	// for(int i=0; i<7; i++){
	// 	for(j=0; j<7; j++){
	// 		cell[i][j].neighbor.clear();
	// 	}
	// }

	/* return the current score */
	printf("current distinct group score is %d.\n", current_score);
	return current_score;
} 


/* find all neighbours with same color and label "visited" */
void visit_neighbour(Cell &cell){
	cell.visit = 1;
	int size = cell.neighbor.size();
	//printf("neighbor size is %d \n", size);

	for(int i=0;i<size;i++){
		//printf("inside for loop\n");
		//printf("for loop i is: %d \n", i);
		//printf("cell color = %d, neighbor[%d] color = %d, neighbor[%d] visit = %d\n", cell.color, i, cell.neighbor[i]->color, i, cell.neighbor[i]->visit);
		if(cell.color == cell.neighbor[i]->color && cell.neighbor[i]->visit != 1){
			//printf("cell color is: %d, neighbor color is: %d \n", cell.color, cell.neighbor[i].color);
			//printf("position x: %d, position y: %d \n", cell.i,cell.j);
			visit_neighbour(*cell.neighbor[i]);
		}
	}
	return;
}


/* minimax algorithm */
int minimax(vector<Slot> slot, int depth, bool isMax){
	if(game_ended() || depth == 0 || !slot.empty()){
		return (evaluate());
	}
	int v;
	if(isMax){
		int v = -100; // initial score
		int size = slot.size();
		for(int i=0; i<size; i++){
			// if exist color different from around color, select different color
			vector<int> color;
			detect_around_color(slot[i].x, slot[i].y, color);
			detect_around_color(slot[i].x_2, slot[i].y_2, color);
			int color_count = 0;
			for(int index=1; index<5; index++){
				vector<int>::iterator it = find(color.begin(), color.end(), index);
				if(it != color.end()){
					color_count++;
					continue; // have the color around
				}
				// the color is not exist around and colorpieces > 0
				else if(colorpieces[index] > 0){
					cell[slot[i].x][slot[i].y].color = index;
					cell[slot[i].x_2][slot[i].y_2].color = index;
					break;
				}	
				else
					color_count++;
			}
			
			// if not exist color different from around color, orderly select one color
			// the color 
			if(color_count == 4){
				for(int i=1; i<5; i++){
					if(colorpieces[i] > 0){
					cell[slot[i].x][slot[i].y].color = i;
					cell[slot[i].x_2][slot[i].y_2].color = i;
					break;
				}
			}
		}
				

			vector<Slot> temp;
			temp = slot;
			temp.erase(temp.begin()+i); // delete slot[i] from vector
			v = max(v, minimax(temp, depth-1, false));
			}
		}

	else{
		int v = 100; // initial score
		int size = slot.size();
		for(int i=0; i<size; i++){
			if(colorpieces[selected_color] > 0){
				// 1st priority color
				cell[slot[i].x][slot[i].y].color = cell[slot[i].x_2][slot[i].y_2].color = selected_color;
			}
			else{ 
			// 2nd priority select around color
				vector<int> color;
				detect_around_color(slot[i].x, slot[i].y, color);
				detect_around_color(slot[i].x_2, slot[i].y_2, color);
				// traverse around color
				int size = color.size();
				for(int i=0; i<size; i++){
					if((color[i] == 1) && (colorpieces[1] > 0)){
						cell[slot[i].x][slot[i].y].color = 1;
						cell[slot[i].x_2][slot[i].y_2].color = 1;
						break;
					}
					else if((color[i] == 2) && (colorpieces[2] > 0)){
						cell[slot[i].x][slot[i].y].color = 2;
						cell[slot[i].x_2][slot[i].y_2].color = 2;
						break;
					}
					else if((color[i] == 3) && (colorpieces[3] > 0)){
						cell[slot[i].x][slot[i].y].color = 3;
						cell[slot[i].x_2][slot[i].y_2].color = 3;
						break;
					}
					else if((color[i] == 4) && (colorpieces[4] > 0)){
						cell[slot[i].x][slot[i].y].color = 4;
						cell[slot[i].x_2][slot[i].y_2].color = 4;
						break;
					}

				}
				color.clear(); // release color
			}

			//slot.erase(slot.begin()+i);
			vector<Slot> temp;
			temp = slot;
			temp.erase(temp.begin()+i); // delete slot[i] from vector
			v = min(v, minimax(temp, depth-1, true));
		}
	}
	printf("I am calculating\n");
	return v; /* how to judge the min or max value in same floor child */
}


/* find nearby available slot for L player */
void find_near_ava_slot_L(int i, int j, int i_2, int j_2, vector<Slot> &slot){
	/* i == i_2 */
	//  _ _ _
	// |4|_5_|
	// |_|_|1|
	// |3|_|_|
	// |_|_2_|

	printf("i:%d, j:%d, i_2:%d, j_2:%d \n", i,j,i_2,j_2);
	Slot temp;
	if(i == i_2){
		// 1 candidate slot
		if((i < 6) && (gameboard[i+1][j] == 0) && (gameboard[i+1][j_2] == 0)){
			temp.x = i + 1;
			temp.y = j;
			temp.x_2 = i + 1;
			temp.y_2 = j_2;
			slot.push_back(temp);
		}
		if(j_2 - j > 0){
			// 2 candidate slot
			if((i < 6) && (j_2 < 6) && (gameboard[i][j_2+1] == 0) && (gameboard[i+1][j_2+1] == 0)){
				temp.x = i;
				temp.y = j_2 + 1;
				temp.x_2 = i + 1;
				temp.y_2 = j_2 + 1;
				slot.push_back(temp);
			}
			// 3 candidate slot
			if((i > 0) && (j_2 < 6) && (gameboard[i-1][j_2] == 0) && (gameboard[i-1][j_2+1] == 0)){
				temp.x = i - 1;
				temp.y = j_2;
				temp.x_2 = i - 1;
				temp.y_2 = j_2 + 1;
				slot.push_back(temp);
			}
			// 4 candidate slot
			if((i > 0) && (j > 0) && (gameboard[i-1][j] == 0) && (gameboard[i-1][j-1] == 0)){
				temp.x = i - 1;
				temp.y = j;
				temp.x_2 = i - 1;
				temp.y_2 = j - 1;
				slot.push_back(temp);
			}
			// 5 candidate slot
			if((i < 6) && (j > 0) && (gameboard[i][j-1] == 0) && (gameboard[i+1][j-1] == 0)){
				temp.x = i;
				temp.y = j - 1;
				temp.x_2 = i + 1;
				temp.y_2 = j - 1;
				slot.push_back(temp);
			}
		}
		else{
			//  _ _ _
			// |4|_5_|
			// |_|_|1|
			// |3|_|_|
			// |_|_2_|
			// 2 candidate slot
			if((i < 6) && (j < 6) &&(gameboard[i][j+1] == 0) && (gameboard[i+1][j+1] == 0)){
				temp.x = i;
				temp.y = j + 1;
				temp.x_2 = i + 1;
				temp.y_2 = j + 1;
				slot.push_back(temp);
			}
			// 3 candidate slot
			if((i > 0) && (j < 6) && (gameboard[i-1][j] == 0) && (gameboard[i-1][j+1] == 0)){
				temp.x = i - 1;
				temp.y = j;
				temp.x_2 = i - 1;
				temp.y_2 = j + 1;
				slot.push_back(temp);
			}
			// 4 candidate slot
			if((i > 0) && (j_2 > 0) && (gameboard[i-1][j_2] == 0) && (gameboard[i-1][j_2-1] == 0)){
				temp.x = i - 1;
				temp.y = j_2;
				temp.x_2 = i - 1;
				temp.y_2 = j_2 - 1;
				slot.push_back(temp);
			}
			// 5 candidate slot
			if((i < 6) && (j_2 > 0) && (gameboard[i][j_2-1] == 0) && (gameboard[i+1][j_2-1] == 0)){
				temp.x = i;
				temp.y = j_2 - 1;
				temp.x_2 = i + 1;
				temp.y_2 = j_2 - 1;
				slot.push_back(temp);
			}
		}
	}	

	/* j == j_2 */
	//  _ _ _ _
	// |5|_1_|2|
	// |_|_|_|_|
	// |_4_|_3_|
	else{
		// 1 candidate slot
		if((j > 0) && (gameboard[i][j-1] == 0) && (gameboard[i_2][j-1] == 0)){
			temp.x = i;
			temp.y = j - 1;
			temp.x_2 = i_2;
			temp.y_2 = j - 1;
			slot.push_back(temp);
		}

		if(i_2 - i > 0){
			// 2 candidate slot
			if((i_2 < 6) && (j > 0) && (gameboard[i_2+1][j] == 0) && (gameboard[i_2+1][j-1] == 0)){
				temp.x = i_2 + 1;
				temp.y = j;
				temp.x_2 = i_2 + 1;
				temp.y_2 = j - 1;
				slot.push_back(temp);
			}
			// 3 candidate slot
			if((i_2 < 6) && (j < 6) && (gameboard[i_2][j+1] == 0) && (gameboard[i_2+1][j+1] == 0)){
				temp.x = i_2;
				temp.y = j + 1;
				temp.x_2 = i_2 + 1;
				temp.y_2 = j + 1;
				slot.push_back(temp);
			}
			// 4 candidate slot
			if((i > 0) && (j < 6) &&(gameboard[i][j+1] == 0) && (gameboard[i-1][j+1] == 0)){
				temp.x = i;
				temp.y = j + 1;
				temp.x_2 = i - 1;
				temp.y_2 = j + 1;
				slot.push_back(temp);
			}
			// 5 candidate slot
			if((i > 0) && (j > 0) && (gameboard[i-1][j] == 0) && (gameboard[i-1][j-1] == 0)){
				temp.x = i - 1;
				temp.y = j;
				temp.x_2 = i - 1;
				temp.y_2 = j - 1;
				slot.push_back(temp);
			}
		}
		else{
			if((i < 6) && (j > 0) && (gameboard[i+1][j] == 0) && (gameboard[i+1][j-1] == 0)){
				temp.x = i + 1;
				temp.y = j;
				temp.x_2 = i + 1;
				temp.y_2 = j - 1;
				slot.push_back(temp);
			}
			// 3 candidate slot
			if((i < 6) && (j < 6) && (gameboard[i][j+1] == 0) && (gameboard[i+1][j+1] == 0)){
				temp.x = i;
				temp.y = j + 1;
				temp.x_2 = i + 1;
				temp.y_2 = j + 1;
				slot.push_back(temp);
			}
			// 4 candidate slot
			if((i_2 > 0) && (j < 6) && (gameboard[i_2][j+1] == 0) && (gameboard[i_2-1][j+1] == 0)){
				temp.x = i_2;
				temp.y = j + 1;
				temp.x_2 = i_2 - 1;
				temp.y_2 = j + 1;
				slot.push_back(temp);
			}
			// 5 candidate slot
			if((i_2 > 0) && (j > 0) && (gameboard[i_2-1][j] == 0) && (gameboard[i_2-1][j-1] == 0)){
				temp.x = i_2 - 1;
				temp.y = j;
				temp.x_2 = i_2 - 1;
				temp.y_2 = j - 1;
				slot.push_back(temp);
			}
		}
	}

	printf("I have found free L slot\n");
	//printf("My free L slot size is %d \n", slot.size());
}

/* find nearby available slot for M player */
void find_near_ava_slot_M(int i, int j, int i_2, int j_2, vector<Slot> &slot){
	/* i == i_2 */
	//    _
	// |_|5|_|
	// |1|_|2|
	// |_|_|_|
	// |4|_|3|
	// |_|6|_|
	//   |_|

	printf("i:%d, j:%d, i_2:%d, j_2:%d \n", i,j,i_2,j_2);
	Slot temp;
	if(i == i_2){
		if(j_2  > j){
			// 1 candidate slot
			if((i > 0) && (j > 0) && (gameboard[i-1][j] == 0) && (gameboard[i-1][j-1] == 0)){
				temp.x = i - 1;
				temp.y = j;
				temp.x_2 = i - 1;
				temp.y_2 = j - 1;
				slot.push_back(temp);
			}
			// 2 candidate slot
			if((i < 6) && (j > 0) && (gameboard[i+1][j] == 0) && (gameboard[i+1][j-1] == 0)){
				temp.x = i + 1;
				temp.y = j;
				temp.x_2 = i + 1;
				temp.y_2 = j - 1;
				slot.push_back(temp);
			}
			// 3 candidate slot
			if((i < 6) && (j_2 < 6) && (gameboard[i+1][j_2] == 0) && (gameboard[i+1][j_2+1] == 0)){
				temp.x = i + 1;
				temp.y = j_2;
				temp.x_2 = i + 1;
				temp.y_2 = j_2 + 1;
				slot.push_back(temp);
			}
			// 4 candidate slot
			if((i > 0) && (j_2 < 6) && (gameboard[i-1][j_2] == 0) && (gameboard[i-1][j_2+1] == 0)){
				temp.x = i - 1;
				temp.y = j_2;
				temp.x_2 = i - 1;
				temp.y_2 = j_2 + 1;
				slot.push_back(temp);
			}
			// 5 candidate slot
			if((j > 1) && (gameboard[i][j-1] == 0) && (gameboard[i][j-2] == 0)){
				temp.x = i;
				temp.y = j - 1;
				temp.x_2 = i;
				temp.y_2 = j - 2;
				slot.push_back(temp);
			}
			// 6 candidate slot
			if((j_2 < 5) && (gameboard[i][j_2+1] == 0) && (gameboard[i][j_2+2] == 0)){
				temp.x = i;
				temp.y = j_2 + 1;
				temp.x_2 = i;
				temp.y_2 = j_2 + 2;
				slot.push_back(temp);
			}
		}
		else{
			// j > j_2
			//    _
			// |_|5|_|
			// |1|_|2|
			// |_|_|_|
			// |4|_|3|
			// |_|6|_|
			//   |_|
			// 1 candidate slot
			if((i > 0) && (j_2 > 0) && (gameboard[i-1][j_2] == 0) && (gameboard[i-1][j_2-1] == 0)){
				temp.x = i - 1;
				temp.y = j_2;
				temp.x_2 = i - 1;
				temp.y_2 = j_2 - 1;
				slot.push_back(temp);
			}
			// 2 candidate slot
			if((i < 6) && (j_2 > 0) && (gameboard[i+1][j_2] == 0) && (gameboard[i+1][j_2-1-1] == 0)){
				temp.x = i + 1;
				temp.y = j_2;
				temp.x_2 = i + 1;
				temp.y_2 = j_2 - 1;
				slot.push_back(temp);
			}
			// 3 candidate slot
			if((i < 6) && (j < 6) && (gameboard[i+1][j] == 0) && (gameboard[i+1][j+1] == 0)){
				temp.x = i + 1;
				temp.y = j;
				temp.x_2 = i + 1;
				temp.y_2 = j + 1;
				slot.push_back(temp);
			}
			// 4 candidate slot
			if((i > 0) && (j < 6) && (gameboard[i-1][j] == 0) && (gameboard[i-1][j+1] == 0)){
				temp.x = i - 1;
				temp.y = j;
				temp.x_2 = i - 1;
				temp.y_2 = j + 1;
				slot.push_back(temp);
			}
			// 5 candidate slot
			if((j_2 > 1) && (gameboard[i][j_2-1] == 0) && (gameboard[i][j_2-2] == 0)){
				temp.x = i;
				temp.y = j_2 - 1;
				temp.x_2 = i;
				temp.y_2 = j_2 - 2;
				slot.push_back(temp);
			}
			// 6 candidate slot
			if((j < 5) && (gameboard[i][j+1] == 0) && (gameboard[i][j+2] == 0)){
				temp.x = i;
				temp.y = j + 1;
				temp.x_2 = i;
				temp.y_2 = j + 2;
				slot.push_back(temp);
			}
		}
	}	

	/* j == j_2 */
	//   _ _ _ _
	// _|_1_|_2_|_
	//|_5_|_|_|_6_|
	//  |_4_|_3_|
	else{
		if(i_2 > i){
			// 1 candidate slot
			if((i > 0) && (j > 0) && (gameboard[i][j-1] == 0) && (gameboard[i-1][j-1] == 0)){
				temp.x = i;
				temp.y = j - 1;
				temp.x_2 = i - 1;
				temp.y_2 = j - 1;
				slot.push_back(temp);
			}
			// 2 candidate slot
			if((i_2 < 6) && (j > 0) && (gameboard[i_2][j-1] == 0) && (gameboard[i_2+1][j-1] == 0)){
				temp.x = i_2;
				temp.y = j - 1;
				temp.x_2 = i_2 + 1;
				temp.y_2 = j - 1;
				slot.push_back(temp);
			}
			// 3 candidate slot
			if((i_2 < 6) && (j < 6) &&(gameboard[i_2][j+1] == 0) && (gameboard[i_2+1][j+1] == 0)){
				temp.x = i_2;
				temp.y = j + 1;
				temp.x_2 = i_2 + 1;
				temp.y_2 = j + 1;
				slot.push_back(temp);
			}
			// 4 candidate slot
			if((i > 0) && (j < 6) && (gameboard[i][j+1] == 0) && (gameboard[i-1][j+1] == 0)){
				temp.x = i;
				temp.y = j + 1;
				temp.x_2 = i - 1;
				temp.y_2 = j + 1;
				slot.push_back(temp);
			}
			// 5 candidate slot
			if((i > 1) && (gameboard[i-1][j] == 0) && (gameboard[i-2][j] == 0)){
				temp.x = i - 1;
				temp.y = j;
				temp.x_2 = i - 2;
				temp.y_2 = j;
				slot.push_back(temp);
			}
			// 6 candidate slot
			if((i_2 < 5) && (gameboard[i_2+1][j] == 0) && (gameboard[i_2+2][j] == 0)){
				temp.x = i_2 + 1;
				temp.y = j;
				temp.x_2 = i_2 + 2;
				temp.y_2 = j;
				slot.push_back(temp);
			}
		}
		else{
			//   _ _ _ _
			// _|_1_|_2_|_
			//|_5_|_|_|_6_|
			//  |_4_|_3_|
			// 1 candidate slot
			if((i_2 > 0) && (j > 0) && (gameboard[i_2][j-1] == 0) && (gameboard[i_2-1][j-1] == 0)){
				temp.x = i_2;
				temp.y = j - 1;
				temp.x_2 = i_2 - 1;
				temp.y_2 = j - 1;
				slot.push_back(temp);
			}
			// 2 candidate slot
			if((i < 6) && (j > 0) && (gameboard[i][j-1] == 0) && (gameboard[i+1][j-1] == 0)){
				temp.x = i;
				temp.y = j - 1;
				temp.x_2 = i + 1;
				temp.y_2 = j - 1;
				slot.push_back(temp);
			}
			// 3 candidate slot
			if((i < 6) && (j < 6) &&(gameboard[i][j+1] == 0) && (gameboard[i+1][j+1] == 0)){
				temp.x = i;
				temp.y = j + 1;
				temp.x_2 = i + 1;
				temp.y_2 = j + 1;
				slot.push_back(temp);
			}
			// 4 candidate slot
			if((i_2 > 0) && (j < 6) && (gameboard[i_2][j+1] == 0) && (gameboard[i_2-1][j+1] == 0)){
				temp.x = i_2;
				temp.y = j + 1;
				temp.x_2 = i_2 - 1;
				temp.y_2 = j + 1;
				slot.push_back(temp);
			}
			// 5 candidate slot
			if((i_2 > 1) && (gameboard[i_2-1][j] == 0) && (gameboard[i_2-2][j] == 0)){
				temp.x = i_2 - 1;
				temp.y = j;
				temp.x_2 = i_2 - 2;
				temp.y_2 = j;
				slot.push_back(temp);
			}
			// 6 candidate slot
			if((i < 5) && (gameboard[i+1][j] == 0) && (gameboard[i+2][j] == 0)){
				temp.x = i + 1;
				temp.y = j;
				temp.x_2 = i + 2;
				temp.y_2 = j;
				slot.push_back(temp);
			}
		}
	}

	printf("I have found free M slot\n");
	//printf("My free M slot size is %d \n", slot.size());
}

/* check the around color */
// need to call 2 times
void detect_around_color(int i, int j, vector<int> &around_color){

	if((i > 0) && (gameboard[i-1][j] != 0) && (gameboard[i-1][j] != 7)){
		around_color.push_back(gameboard[i-1][j]);
	}
	if((i < 6) && (gameboard[i+1][j] != 0) && (gameboard[i+1][j] != 7)){
		around_color.push_back(gameboard[i+1][j]);
	}
	if((j > 0) && (gameboard[i][j-1] != 0) && (gameboard[i][j-1] != 7)){
		around_color.push_back(gameboard[i][j-1]);
	}
	if((j < 6) && (gameboard[i][j+1] != 0) && (gameboard[i][j+1] != 7)){
		around_color.push_back(gameboard[i][j+1]);
	}
}

/* find one available slot in the board */
vector<int> find_one_slot(){
	vector<int> pos;
	for(int i=0; i<7; i++){
		for(int j=0; j<7; j++){
			if((i > 0) && (gameboard[i][j] == 0) && (gameboard[i-1][j] == 0)){
				pos.push_back(i);
				pos.push_back(j);
				pos.push_back(i-1);
				pos.push_back(j);
				return pos;
			}
			if((i < 6) && (gameboard[i][j] == 0) && (gameboard[i+1][j] == 0)){
				pos.push_back(i);
				pos.push_back(j);
				pos.push_back(i+1);
				pos.push_back(j);
				return pos;
			}
			if((j > 0) && (gameboard[i][j] == 0) && (gameboard[i][j-1] == 0)){
				pos.push_back(i);
				pos.push_back(j);
				pos.push_back(i);
				pos.push_back(j-1);
				return pos;
			}
			if((i < 6) && (gameboard[i][j] == 0) && (gameboard[i][j+1] == 0)){
				pos.push_back(i);
				pos.push_back(j);
				pos.push_back(i);
				pos.push_back(j+1);
				return pos;
			}
		}
	}
}
