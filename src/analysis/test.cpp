#include "piece.hpp"
#include "edge.hpp"
#include "find_pieces.hpp"
#include "edge_tools.hpp"
#include "user_gui.hpp"

#include <string>
#include <vector>
#include <cstdio>
#include <cmath>
#include <functional>
#include <utility>
#include <algorithm>
#include <opencv2/opencv.hpp>

using namespace std;

vector<Edge> find_to_compare( vector<Piece> pieces, Edge match_to )
   {
   vector<Edge> to_return;
   size_t num = match_to.types.size();

   cout << "Looking for ";
   for( size_t i = 0; i < num; i++ )
      cout << match_to.types[i] << ", ";
   cout << endl;

   // For ever piece
   size_t piece_index = 0;
   for( Piece piece : pieces )
      {
      cout << "Looking at piece: " << to_string(piece_index++) << endl;

      // Starting at every curve in piece
      bool to_add = true;
      vector<Curve> potential;
      for( size_t curve_index = 0; curve_index < piece.curves.size(); curve_index++ )
         {
         cout << "Checking curve: " << to_string( curve_index ) << endl;

         potential.clear();
         to_add = true;
         size_t num_curves = piece.curves.size();

         // Stop if we don't have enough curves
         if( num_curves < num )
            break;

         // Check needed number of curves in advance
         for( size_t j = 0; j < num; j++ )
            {
            cout << "Looking for " << match_to.types[j] << " found " << piece.curves[(j+curve_index)%num_curves].type << endl;

            // Prematurely add
            potential.push_back( piece.curves[(j+curve_index)%num_curves]);
            if( piece.curves[(j+curve_index)%num_curves].type != match_to.types[j] )
               {
               // Required sequence broken
               to_add = false;
               break;
               }
            }
         if( to_add )
            {
            cout << "Found potential edge! Adding edge " << to_string(curve_index) << " from piece " << to_string(piece_index) << endl;
            Edge new_edge(piece, potential );
            to_return.push_back( new_edge );
            }
         }
      }
   return to_return;
   }


Piece piece_to_fake( Piece input, size_t start_idx, size_t end_idx )
   {
   // working points
   cv::Point2f start = input.contour[start_idx];
   cv::Point2f end = input.contour[end_idx];
   cv::Point2f corner;
   cv::Point2f temp_pt;

   // length of corner we are creating
   float length = sqrt( pow(start.x - end.x, 2) + pow(start.y - end.y, 2 ) ) / sqrt(2);

   // Starting point, along X axis with offset
   corner.x = start.x + length;
   corner.y = start.y;

   // rotate 45 degrees from existing line
   float angle = -45.0;

   temp_pt = cv::Point2f(end);
   temp_pt.x += 0-start.x; temp_pt.y += 0-start.y;

   angle += -1*atan2(temp_pt.y, temp_pt.x)*180.0/M_PI;
   // rotate be amount existing line is from normal
   cv::Mat trans = cv::getRotationMatrix2D( start, angle, 1 );
   vector<cv::Point> to_transform(1, corner );
   vector<cv::Point> transformed;
   cv::transform( to_transform, transformed, trans );

   Piece to_return;

   // Copy over selected points
   to_return.contour = vector<cv::Point>( &(input.contour[start_idx]), &(input.contour[end_idx]) );
   to_return.points = vector<Vec2d>( &(input.points[start_idx]), &(input.points[end_idx]) );

   // Flip order, because we are esentially flipping piece inside out
   reverse(to_return.contour.begin(), to_return.contour.end() );
   reverse(to_return.points.begin(), to_return.points.end() );

 // Add new point
   to_return.contour.push_back( transformed[0] );
   to_return.points.push_back( Vec2d(transformed[0].x, transformed[0].y ) );

   // Find inflection points, process
   vector<size_t> infl_idx = find_inflections( to_return.points );
   to_return.set_inflection( infl_idx );
   to_return.process();

   return to_return;
   }

vector<Vec2d> gen_curve(int N, double start, double end, function<double(double)> func) {
    vector<Vec2d> points;
    double range = end - start;
    for (int i = 0; i < N; i++) {
        double x = range * (double(i) / N) + start;
        points.push_back(Vec2d(x, func(x)));
    }
    return points;
};

double identity(double x) {return x;}

// 3x^4 - x^3 - 5x^2 + x
// inflection points should be at -0.85, 0.01, 1.0
double polynomial(double x) {
    return 3*pow(x, 4) - pow(x, 3) - 5*pow(x, 2) + x;
}

double whatever_func(double x) {
    return sin(3*x);
}

void test_function(void)
   {

    int N = 48;
    vector<Vec2d> points = gen_curve(N, -1.25, 1.4, polynomial);
    auto infl_indices = find_inflections(points);
    // cv::Mat img = draw_curve(points, 480, infl_indices, vector<size_t>(), true);
    // cv::namedWindow("AWESOME", cv::WINDOW_AUTOSIZE);
    // cv::imshow("AWESOME", img);
    // cv::waitKey(0);

   }


vector<cv::Mat> obtain_images(int camera_num) {
   vector<cv::Mat> images;
   cv::VideoCapture cap;
   if (!cap.open(camera_num)) {
      cout << "Failed to open webcam" << endl;
      return images;
   }

   while (1) {
      cv::Mat frame;
      cap >> frame;
      // cv::Mat blue(frame.rows, frame.cols, CV_8UC1);
      // cv::Mat green(frame.rows, frame.cols, CV_8UC1);
      // cv::Mat red(frame.rows, frame.cols, CV_8UC1);
      // cv::Mat channels[3] = {blue, green, red};
      // cv::split(frame, channels);
      // cv::equalizeHist(blue, blue);
      // cv::equalizeHist(green, green);
      // cv::equalizeHist(red, red);
      // cv::merge(channels, 3, frame);
      cv::imshow("webcam", frame);
      char key = cv::waitKey(30);
      // Enter key
      if (key == 10) {
         return images;
      }
      if (key == ' ') {
         cv::Mat frame_dup = frame.clone();
         images.push_back(frame_dup);
      }
      cout << (int)key << endl;
   }
}

void test_pieces(vector<cv::Mat> images) {
   vector<Piece> pieces;

   vector<Piece> partials;
    for( cv::Mat image : images )
       {
       vector<Piece> temp;
       vector<Piece> found = find_pieces( image, temp );
       pieces.insert( pieces.end(), found.begin(), found.end() );
       partials.insert( partials.end(), temp.begin(), temp.end() );
      }
   cout << "Found " << pieces.size() << " pieces." << endl;
   cout << "Found " << partials.size() << " partials." << endl;
   if( partials.size() != 1 )
      {
      cout << "Too many partials. Exiting." << endl;
      return;
      }

   // Pieces matching to
   //   cv::Mat test_img = cv::imread( "../../images/camera/cam_partial_180.png", 1 );
   // cv::Mat test_img = cv::imread( "../../images/pieces/test_1_2.png", 1 );
   // vector<Piece> match_to_vec = find_pieces( partials[0] );
   // Piece match_to = match_to_vec[0];
   Piece match_to = partials[0];

   // Create GUI object
   PuzzleGUI gui("User GUI");

   // Hold found inflection points
   vector<size_t> infl_indices;
   vector<Vec2d> infl_points;

   for( size_t i = 0; i < pieces.size(); i++ )
      {

      // Find inflection points and process pieces
      infl_indices = find_inflections(pieces[i].points, ((M_PI / 180) * 40));
      pieces[i].set_inflection( infl_indices );
      pieces[i].process();

      // auto straight_lines = find_straight_sides(pieces[i].points, ((M_PI / 180) * 0.5));

      // Draw processed pieces to screen
      // pieces[i].draw( 480 );
      // cout << "Showing image " << i << endl;
      // while(cv::waitKey(30) != ' ' );

      }

   // // Prompt user for image
   pair<size_t, size_t> selection = gui.select_edge( match_to );

   // // Extract first and last points from selected region
   size_t start_idx = get<0>(selection);
   size_t end_idx = get<1>(selection);

   // Create "fake piece" from selection
   Piece fake = piece_to_fake( match_to, start_idx, end_idx );

   // Show fake piece to screen
//    fake.draw( 480 );
//    while(cv::waitKey(30) != ' ' );

   // Create edge from faked piece
   Edge match_edge( fake, fake.curves );

   // Find edges from remaining pieces that could match
   vector<Edge> potential;
   potential = find_to_compare( pieces, match_edge );
   cout << "Found " << potential.size() << " edges to try." << endl;
   highlight_matches(match_edge, potential);
}

int main(int argc, char *argv[])
   {
      // Using webcam
   if ( (argc == 2 || argc == 3 ) && strcmp(argv[1], "webcam") == 0)
      {
      int camera_num = 0;
      if( argc == 3 )
         {
         sscanf(argv[2], "%d", &camera_num );
         }
         auto images = obtain_images( camera_num );
         cout << "Using " << images.size() << " images" << endl;
         test_pieces(images);
      }
      else
      {
         // Pieces to process
         vector<cv::Mat> images;
         //    images.push_back( cv::imread("../../images/camera/cam_masked.png", 1 ));
         images.push_back(cv::imread("../../images/camera/large_closeup_partial.jpg", 1));
         images.push_back(cv::imread("../../images/camera/large_closeup_pieces1.jpg", 1));
         images.push_back(cv::imread("../../images/camera/large_closeup_pieces2.jpg", 1));
         //    images.push_back( cv::imread("../../images/rows/row1_shrunk.png", 1 ));
         //    images.push_back( cv::imread("../../images/rows/row2_shrunk.png", 1 ));
         //    images.push_back( cv::imread("../../images/rows/row3_shrunk.png", 1 ));
         //    images.push_back( cv::imread("../../images/rows/row4_shrunk.png", 1 ));
         test_pieces(images);
      }
      return 0;
   };
