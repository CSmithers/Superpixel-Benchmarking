#include <iostream>
#include <vector>
#include <algorithm>
#include <cmath>
#include <fstream>
#include <string>
#include <sstream>

using namespace std;


//Hard-coded variables
const string Document1 = "33039.seg";
const string Document2 = "61086.seg";
const int epsilon = 2;


class segmentation
{
    public:
        int totalSegments = 0;
        vector<int> segNumber = {}, rowNumber = {}, startPixel = {}, endPixel = {};
};
/*
Defines a "segmentation", dividing a rectangular pixel grid into regions.
    Total Segments = the number of regions into which the image is divided (perhaps not connected!)
    The tuple: segNumber[i] rowNumber[i] startPixel[i] endPixel[i] should be interpretted as a horizontal line of pixels in line rowNumber[i]
    starting at startPixel[i] and ending at endPixel[i], belonging to segment segNumber[i].
    e.g   2, 3, 4, 7 is a line of pixels of width 4, on row 3, starting at pixel 4 and ending at 7, belonging to segment 2
*/

bool comparable(segmentation testSegmentation, segmentation groundTruth)
{
    bool isComparable = true;
    if (testSegmentation.endPixel.back() != groundTruth.endPixel.back())
    {
        cout << "Warning: The segmentations are for images of different widths" << endl;
        isComparable = false;
    }
    if (testSegmentation.rowNumber.back() != groundTruth.rowNumber.back())
    {
        cout << "Warning: The segmentations are for images of different heights" << endl;
    }
    if (!isComparable)
    {
        cout << "Segmentations were not comparable, the program will now terminate" << endl << endl;
    }
    return isComparable;
}
///comparable(.,.) checks whether the two segmentations are valid segmentations of images of the same dimensions, returning the obvious boolean value and warning the user

vector<vector<int>> intersections(segmentation testSegmentation, segmentation groundTruth)
{
    bool flag = true;
    vector<vector<int>> intersections(testSegmentation.totalSegments, vector<int>(groundTruth.totalSegments, 0));
    unsigned int testEntry = 0;
    unsigned int groundEntry = 0;
    //Initialise

    while (flag)
    {
        int currentTestRegion[] = {testSegmentation.segNumber[testEntry],
            testSegmentation.rowNumber[testEntry],
            testSegmentation.startPixel[testEntry],
            testSegmentation.endPixel[testEntry]
        };//Define the current portion of the test segmentation
        int currentGroundRegion[4] = {groundTruth.segNumber[groundEntry],
            groundTruth.rowNumber[groundEntry],
            groundTruth.startPixel[groundEntry],
            groundTruth.endPixel[groundEntry]
        };//Define the current portion of the ground truth segmentation

        int intersectionSize = min(currentTestRegion[3],currentGroundRegion[3]) - max(currentTestRegion[2],currentGroundRegion[2]);
        intersections[currentTestRegion[0]][currentGroundRegion[0]] = intersectionSize + 1 + intersections[currentTestRegion[0]][currentGroundRegion[0]];
        //Assign the appropriate intersection value

        if ((currentTestRegion[3] == currentGroundRegion[3]))//End pixels are the same, change both
        {
            testEntry++;
            groundEntry++;
            if (groundEntry == groundTruth.segNumber.size())//Do the two regions share an endpoint, including the end of a row?
            {
                flag = false;// Are we beyond the bottom of the picture?
            }
        }
        else if (currentTestRegion[3] < currentGroundRegion[3])
        {
            testEntry++;
        }
        else
        {
            groundEntry++;
        }

    }

    return intersections;
}
///intersections(.,.) takes two segmentations, say T and G, and outputs A(i,j) = intersection (# of pixels) of T_i and G_j

float undersegError(segmentation testSegmentation, segmentation groundTruth, vector<vector<int>> intersections)
{
    int pixelNum = (1 + groundTruth.rowNumber.back())*(1 + groundTruth.endPixel.back()); //Calculate pixel number by multiplying the height and width
    cout << "There are " << pixelNum << " Pixels" << endl;
    //Multiply the row number of the final segment entry by the column number of the final segment entry to get number of pixels in the image

    int errorTerm = 0;//initialise the error sum at 0
    for (int groSeg = 0; groSeg < groundTruth.totalSegments; groSeg++)
    {
        for (int testSeg = 0; testSeg < testSegmentation.totalSegments; testSeg++)//For each non-zero value, add all other entries in that row
        {
            if (intersections[testSeg][groSeg] > 0)
            {
                errorTerm = errorTerm - intersections[testSeg][groSeg];
                for (int iter = 0; iter < groundTruth.totalSegments; iter++)
                {
                    errorTerm = errorTerm + intersections[testSeg][iter];// For simplicity, we edd all entries in the row and subtract the entry we are currently looking at
                }
            }
        }
    }

    float totalError = (float) errorTerm/(float) pixelNum;
    return totalError;
}
///undersegError(.,.) computes the Undersegmentation error of a super-pixel segmentation, when benchmarked against ground truth

float boundaryRecall(segmentation testSegmentation, segmentation groundTruth, int epsilon)
{
    vector<vector<int>> testBoundary (1+testSegmentation.rowNumber.back(), vector<int>(1+testSegmentation.endPixel.back(), 0));
    //We build a mask with 1s on boundary pixels and 0s elsewhere
    for (unsigned int iter = 0; iter < testSegmentation.endPixel.size(); iter++ )//Iterate over the horizontal regions
    {
        testBoundary[testSegmentation.rowNumber[iter]][testSegmentation.startPixel[iter]] = 1;//Add the left end to boundary
        testBoundary[testSegmentation.rowNumber[iter]][testSegmentation.endPixel[iter]] = 1;//Add the right end to boundary
    }

    int groundBoundarySize = 0;
    int groundBoundaryRecovered = 0;

    for (unsigned int iter = 0; iter < groundTruth.endPixel.size(); iter++)//Iterate over the horizontal ground truth regions
    {
        groundBoundarySize++;
        bool flag = false;

        int startX = max(0,groundTruth.rowNumber[iter] - epsilon);
        int startY = max(0, groundTruth.startPixel[iter] - epsilon);
        int endX = min(groundTruth.rowNumber.back(),groundTruth.rowNumber[iter] + epsilon);
        int endY = min(groundTruth.endPixel.back(),groundTruth.startPixel[iter] + epsilon);
        //Define the rectangle of pixels within epsilon of the relevant boundary point (left hand end of region)


        for (int it1 = startX; it1 <= endX; it1++)
        {
            for (int it2 = startY; it2 <= endY; it2++)
            {
                if( testBoundary[it1][it2] == 1)
                {
                flag = true;
                }
            }
        }
        if (flag)
        {
            groundBoundaryRecovered++;
        }
        //If the boundaries are within epsilon as desired, add to a count

        //Repeat for the right end of the region unless the left and right coincide (region is 1 pixel thinck
        if (groundTruth.startPixel[iter] != groundTruth.endPixel[iter])
        {
            groundBoundarySize++;
            flag = false;

            startX = max(0,groundTruth.rowNumber[iter] - epsilon);
            startY = max(0, groundTruth.endPixel[iter] - epsilon);
            endX = min(groundTruth.rowNumber.back(),groundTruth.rowNumber[iter] + epsilon);
            endY = min(groundTruth.endPixel.back(),groundTruth.endPixel[iter] + epsilon);
            for (int it1 = startX; it1 <= endX; it1++)
            {
                for (int it2 = startY; it2 <= endY; it2++)
                {
                    if( testBoundary[it1][it2] == 1)
                    {
                    flag = true;
                    }
                }
            }
            if (flag)
            {
                groundBoundaryRecovered++;
            }
        }
    }
    float totalRecall = (float) groundBoundaryRecovered/(float) groundBoundarySize;
    return totalRecall;
}
///boundaryRecall(.,.,.) computes the Boundary Recall error of a super-pixel segmentation, with width epsilon, when benchmarked against ground truth
///Note :   i) We assume that "within epsilon pixels" includes diagonal movements of pixels
///        11) As it stands, we miss the bottom and top (flat) boundaries

float achievableSegmentationAccuracy(segmentation testSegmentation, segmentation groundTruth, vector<vector<int>> intersections)
{
    int pixelNum = (1+groundTruth.rowNumber.back())*(1 + groundTruth.endPixel.back());
    int accuracyTerm = 0;

    for (int iter = 0; iter < testSegmentation.totalSegments; iter++){
        vector<int> row = intersections[iter];
        accuracyTerm += *max_element(row.begin(),row.end());// Add the maximal value in each row to our sum
    }
    float totalAccuracy = (float) accuracyTerm/(float) pixelNum;

    return totalAccuracy;
}
///achievableSegmentationAccuracy(.,.) computes the Achievable segmentation accuracy of a super-pixel segmentation, when benchmarked against ground truth

int main()
{
    segmentation testSeg;
    segmentation groundSeg;

    /* Test/Example data
    testSeg.segNumber = {0,0,0,0,0,1,1,1,1,1};
    testSeg.rowNumber = {0,1,2,3,4,5,6,7,8,9};
    testSeg.startPixel = {0,0,0,0,0,0,0,0,0,0};
    testSeg.endPixel = {9,9,9,9,9,9,9,9,9,9};
    testSeg.totalSegments = 2;

    groundSeg.segNumber = {0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,1};
    groundSeg.rowNumber = {0,0,1,1,1,2,2,2,3,3,4,4,5,5,6,6,7,7,8,8,9,9};
    groundSeg.startPixel = {0,5,0,5,0,5,0,5,0,5,0,5,0,5,0,5,0,5,0,5};
    groundSeg.endPixel = {4,9,4,9,4,9,4,9,4,9,4,9,4,9,4,9,4,9,4,9};
    groundSeg.totalSegments = 2;

    testSeg.segNumber = {1,2,0,1,2,0,1,2,0,1,0,1};
    testSeg.rowNumber = {0,0,1,1,1,2,2,2,3,3,4,4};
    testSeg.startPixel = {0,2,0,1,3,0,2,4,0,3,0,4};
    testSeg.endPixel = {1,4,0,2,4,1,3,4,2,4,3,4};
    testSeg.totalSegments = 3;

    groundSeg.segNumber = {0,1,0,1,2,0,1,2,1,2,1,2};
    groundSeg.rowNumber = {0,0,1,1,1,2,2,2,3,3,4,4};
    groundSeg.startPixel = {0,3,0,2,4,0,1,3,0,2,0,1};
    groundSeg.endPixel = {2,4,1,3,4,0,2,4,1,4,0,4};
    groundSeg.totalSegments = 3;
 */

    string line;
    ifstream myfile(Document1);//Open the first file
    bool file1finished = false;
    if (myfile.is_open())
    {
        while ( getline (myfile,line) )
        {
            if (file1finished)
            {
                istringstream iss(line);
                int sub;
                iss >> sub;
                testSeg.segNumber.push_back(sub);
                iss >> sub;
                testSeg.rowNumber.push_back(sub);
                iss >> sub;
                testSeg.startPixel.push_back(sub);
                iss >> sub;
                testSeg.endPixel.push_back(sub);
                //Load values into testSeg
            }

            if (line == "data")
            {
                file1finished = true;//Ignore everything before "data"
            }

        }
        myfile.close();
    }
    testSeg.totalSegments = *max_element(testSeg.segNumber.begin(), testSeg.segNumber.end()) + 1;//Initialise the number of segments

    bool file2finished = false;
    ifstream myfile2(Document2);//Open the second file
    if (myfile2.is_open())
    {
        while ( getline (myfile2,line) )
        {
            if (file2finished)
            {
                istringstream iss(line);
                int sub;
                iss >> sub;
                groundSeg.segNumber.push_back(sub);
                iss >> sub;
                groundSeg.rowNumber.push_back(sub);
                iss >> sub;
                groundSeg.startPixel.push_back(sub);
                iss >> sub;
                groundSeg.endPixel.push_back(sub);
                //Load values into groundSeg
            }

            if (line == "data")
            {
                file2finished = true;//Ignore everything before "data"
            }
        }
        myfile2.close();
    }
    groundSeg.totalSegments = *max_element(groundSeg.segNumber.begin(),groundSeg.segNumber.end()) + 1;//Initialise the number of segments

    bool isComparable = comparable(testSeg,groundSeg);

    if (isComparable){
        vector<vector<int>> intersects = intersections(testSeg, groundSeg);
        float Uerror = undersegError(testSeg,groundSeg, intersects);
        cout << "Undersegmentation Error = " << Uerror << endl;
        float accuracy = achievableSegmentationAccuracy(testSeg, groundSeg, intersects);
        cout << "Achievable Segmentation Accuracy = " << 100*accuracy << "%" <<  endl;
        float recall = boundaryRecall(testSeg, groundSeg, epsilon);
        cout << "Boundary Recall percentage = " << 100*recall << "%" <<  endl;
    }
    return 0;
}
