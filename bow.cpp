#include <iostream>
#include <vector>
#include <string>

#include <opencv2/opencv.hpp>
#include <opencv2/xfeatures2d.hpp>
#include <opencv2/ml.hpp>

using namespace std;
using namespace cv;
using namespace cv::ml;
using namespace cv::xfeatures2d;

const int vocSize = 25;
const int numOfTrees = 500;

// обучение словоря
Mat trainVocabulary(const vector<string>& filesList, const Ptr<Feature2D>& keyPointsDetector) {
    Mat img, descriptors;
    vector<KeyPoint> keyPoints;
    BOWKMeansTrainer tr(vocSize);

    cout << "\nTrain vocabulary\n";
    for (int i = 0; i < filesList.size(); i++) {
        //cout << "\r" << (float)(i) / (float)(filesList.size()) * 100 << "%";

        img = imread(filesList[i], IMREAD_GRAYSCALE);

        keyPointsDetector->detect(img, keyPoints);
        keyPointsDetector->compute(img, keyPoints, descriptors);

        tr.add(descriptors);
    }

    //cout << "Wait...";
    Mat temp = tr.cluster();
    //cout << "\n";
    return temp;
}

// возвращает признаковое описание изображения
Mat extractFeaturesFromImage(Ptr<Feature2D> keyPointsDetector, Ptr<BOWImgDescriptorExtractor> bowExtractor,
                             const string& fileName) {
    Mat imgDescriptor, keyPointsDescriptors;
    Mat img = imread(fileName, IMREAD_GRAYSCALE);
    vector<KeyPoint> keyPoints;

    keyPointsDetector->detect(img, keyPoints);

    keyPointsDetector->compute(img, keyPoints, keyPointsDescriptors);
    bowExtractor->compute(keyPointsDescriptors, imgDescriptor);

    return imgDescriptor;
}

// формирование обучающей выборки
void extractTrainData(const vector<string>& filesList, const Mat& responses, Mat& trainData, Mat& trainResponses,
                      const Ptr<Feature2D>& keyPointsDetector, const Ptr<BOWImgDescriptorExtractor>& bowExtractor) {
    trainData.create(0, bowExtractor->descriptorSize(), CV_32F);
    trainResponses.create(0, 1, CV_32S);

    cout << "Formation of training sample\n";
    for (int i = 0; i < filesList.size(); i++) {
        //cout << "\r" << i/filesList.size()*100 << "%";

        trainData.push_back(extractFeaturesFromImage(keyPointsDetector, bowExtractor, filesList[i]));
        trainResponses.push_back(responses.at<int>(i));
    }

    //cout << "\n";
}

// обучение классификатора «случайный лес»
Ptr<RTrees> trainClassifier(const Mat& trainData, const Mat& trainResponses) {
    Ptr<RTrees> rTrees;

    rTrees = ml::RTrees::create();
    rTrees->setTermCriteria(TermCriteria(TermCriteria::COUNT, numOfTrees, 0));

    Ptr<TrainData> tData = TrainData::create(trainData, ROW_SAMPLE, trainResponses);

    rTrees->train(tData);

    return rTrees;
}

// возвращает набор предсказанных значений для тестовой выборки
Mat predictOnTestData(const vector<string>& filesList, const Ptr<Feature2D> keyPointsDetector,
                      const Ptr<BOWImgDescriptorExtractor> bowExtractor, const Ptr<RTrees> classifier) {
    Mat answers(0, 1, CV_32F);

    cout << "Prediction class";
    for (int i = 0; i < filesList.size(); i++) {
       // cout << "\r" << i/filesList.size()*100 << "%";

        Mat description = extractFeaturesFromImage(keyPointsDetector, bowExtractor, filesList[i]);
        float qwe = classifier->predict(description);
        answers.push_back(qwe);
    }

    cout << "\n\n";
    return answers;
}
