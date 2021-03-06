#include "opencv2/highgui/highgui.hpp"
#include "opencv2/imgproc/imgproc.hpp"
#include <iostream>
#include <stdio.h>
#include <stdlib.h>

using namespace cv;
using namespace std;

int main( int argc, char* argv[] )
   {
   // is user dumb
   if( argc != 2 )
      {
      cout << "Please include image." << endl;
      exit(-1);
      }

   // read in image
   Mat src = imread(argv[1], 1 );

   // convert to BW
   cvtColor( src, src, CV_BGR2GRAY );
   src = src > 100;
   imwrite("../../images/out/get_points_out_bw.png", src );

   Mat temp = src.clone();
   // Vector of groups of contours
   vector<vector<Point>> contours;

   double threshold = 10000.0;
   double size_factor = 2.0;


   // CV_CHAIN_APPROX_NONE stores absolutely all the contour points.
   //  That is, any 2 subsequent points (x1,y1) and (x2,y2) of the contour
   //  will be either horizontal, vertical or diagonal neighbors,
   //  that is, max(abs(x1-x2),abs(y2-y1))==1.
   // CV_CHAIN_APPROX_SIMPLE compresses horizontal, vertical, and
   //  diagonal segments and leaves only their end points. For
   //  example, an up-right rectangular contour is encoded with 4 points.
   // CV_CHAIN_APPROX_TC89_L1,CV_CHAIN_APPROX_TC89_KCOS applies one of
   //  the flavors of the Teh-Chin chain approximation algorithm.
   cout << "Finding contours" << endl;
   findContours(temp, contours, CV_RETR_EXTERNAL, CV_CHAIN_APPROX_NONE);

   // White canvas to output onto
   Mat output(src.size(), CV_8UC1, Scalar(255,255,255));

   //find small contours and ignore them
   cout << "Finding large contours.\nFound " << contours.size() << " contours." << endl;
   double average_size = 0.0;
   int i;
   for(i = 0; i < contours.size(); i++ )
      {
      cout << "Checking contour: " << i << endl;
      // Discount anything that is smaller thatn 10 000
      if( contourArea(contours[i]) < threshold )
         {
         //Erase garbage
         cout << "Erasing contour " << i << endl;
         contours.erase(contours.begin() + i);
         i--;
         continue;
         }
      cout << "\n\tHas area: " << contourArea(contours[i]) << endl;
      cout << "\n\tHas size: " << contours[i].size() << endl;
      average_size += contourArea(contours[i]);

      }

   // Calculate average size of large-ish objects
   average_size /= i;

   cout << "Found average size to be: " << average_size << endl;

   // remove abnormally sized objects
   for(i = 0; i < contours.size(); i++ )
      {
      if( contourArea(contours[i]) > (average_size*size_factor) )
         {
         contours.erase(contours.begin() + i );
         i--;
         }
      }

   cout << "Drawing contours" << endl;

   Scalar color( rand()&200, rand()&200, rand()&200 );
   //Scalar color( 0, 0, 0 );
   drawContours( output, contours, -1, color, CV_FILLED);

   // Display image
   char* test_win = "Test";
   namedWindow( test_win, CV_WINDOW_NORMAL );
   imshow( test_win, output );
   resizeWindow( test_win, 400, 800 );

   // Write output to file
   imwrite("../../images/out/get_points_out.png", output );

   waitKey(0);


   return 0;
   }
