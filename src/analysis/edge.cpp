#include "edge.hpp"
#include <math.h>
#include <iostream>
#define RAD_TO_DEG (180.0 / M_PI)

using namespace std;
using namespace cv;

Edge::Edge() {}

Edge::Edge(Piece piece, vector<Curve> curves)
{
    this->origin = curves[0].origin;

    if (curves.size() >= 1)
        this->handle = curves[curves.size() - 1].origin;

    this->points = vector<Point>(
        &(piece.contour[curves[0].start]),
        &(piece.contour[curves[curves.size() - 1].end]));

    for (Curve curve : curves)
        this->types.push_back(curve.type);
}

void Edge::translate(void)
{
    // Translate wrt origin
    float delta_x = 0 - origin.x;
    float delta_y = 0 - origin.y;
    for (Point pt : points)
    {
        pt.x += delta_x;
        pt.y += delta_y;
    }
    handle.x += delta_x;
    handle.y += delta_y;
}

void Edge::rotate(Point that)
{
    // Rotate wrt point and handle

    float angle = this->handle.dot(that);

    float len1 = sqrt(this->handle.x * this->handle.x + this->handle.y * this->handle.y);
    float len2 = sqrt(that.x * that.x + that.y * that.y);

    float a = this->handle.dot(that) / (len1 * len2);

    angle = (a >= 0 ? 1.0 : -1.0) * acos(a);

    Mat transformation = getRotationMatrix2D(this->origin, angle, 1);
    transform(this->points, this->points, transformation);
}

float Edge::compare(const Edge &that)
{
    // How much to translate the points by. Moves "that" to the origin of "this"
    Point translate_amt = origin - that.origin;
    // Find how much to rotate that by after translation so origins match
    double this_angle = atan2(origin.y - handle.y, handle.x - origin.x);
    double that_angle = atan2(that.origin.y - that.handle.y, that.handle.x - that.origin.x);
    double amt_to_rotate = this_angle - that_angle;
    // Matrix to rotate "that" about its origin by the correct amount
    Mat rot_mat = getRotationMatrix2D(Point(0, 0), amt_to_rotate * RAD_TO_DEG, 1);
    // Matrix to translate points
    double data[2][3] = {{1, 0, translate_amt.x}, {0, 1, translate_amt.y}};
    Mat trans_mat(2, 3, CV_64F, data);

    // Translate then rotate
    vector<Point> moved_pts;
    transform(that.points, moved_pts, trans_mat);
    transform(moved_pts, moved_pts, rot_mat);

    // optimal transform from the moved and rotated "that" to "this"
    // cv::Mat opt_tran = estimateRigidTransform(moved_pts, points, false);
    Mat opt_tran(3, 4, CV_64F);
    // vector<Point3f> inliers;
    vector<Point3f> this_points3;
    vector<Point3f> that_points3;
    // for (Point p : points) {
    //     this_points3.push_back(Point3i(p.x, p.y, 0));
    // }
    // for (Point p : that.points) {
    //     that_points3.push_back(Point3i(p.x, p.y, 0));
    // }
    Mat a = Mat(points, CV_32F);
    Mat b = Mat(that.points, CV_32F);
    Mat inliers;
    // Mat a3(a.size(), CV_MAKE_TYPE(a.type(), 3));
    Mat a3(a.size(), CV_MAKE_TYPE(a.type(), 3));
    Mat b3(b.size(), CV_MAKE_TYPE(b.type(), 3));
    int copy_method[4] = {0,0, 1,1};
    mixChannels(&a, 1, &a3, 1, copy_method, 2);
    mixChannels(&b, 1, &b3, 1, copy_method, 2);
    cout << "Checkvec: " << a3.checkVector(3) << endl;
    cout << "Checkvec: " << b3.checkVector(3) << endl;
    estimateAffine3D(a3, b3, opt_tran, inliers);
    cout << "opt_tran = " << endl << opt_tran << endl;
    return 0;
}