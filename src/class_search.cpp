#include "class_search.h"

SearchInterface search_intf;

SearchInterface::SearchInterface(){};

//"yolomodels/yolov5s-visdrone.onnx"
void SearchInterface::Init() {
    std::vector<cv::Scalar> colors = {cv::Scalar(255, 255, 0), cv::Scalar(0, 255, 0), cv::Scalar(0, 255, 255), cv::Scalar(255, 0, 0)};
    std::vector<std::string> class_list;
    cv::Size procSize((double)cap_intf.frameSize.width * track_intf.image_scale, (double)cap_intf.frameSize.height * track_intf.image_scale);
    if (settings.searchType==1) {
        std::cout << "Loading net: "<<settings.search_dnn_model<<std::endl;
        auto result = cv::dnn::readNet(settings.search_dnn_model);

        if (settings.use_cuda)    {
            std::cout << "Attempting to use CUDA\n";
            result.setPreferableBackend(cv::dnn::DNN_BACKEND_CUDA);
            result.setPreferableTarget(cv::dnn::DNN_TARGET_CUDA_FP16);
            std::cout << "Loaded" << std::endl;
        }    else    {
            std::cout << "Running on CPU\n";
            result.setPreferableBackend(cv::dnn::DNN_BACKEND_OPENCV);
            result.setPreferableTarget(cv::dnn::DNN_TARGET_CPU);
        }
        net = result;
        if (result.empty()) {
            std::cerr << "Error: Model not loaded, net empty" << std::endl;
            global_running.store(false);
            return;
        }
        std::vector<std::string> class_list = search_intf.load_class_list();
        setup = true;
    }
};

//bool SearchInterface::InitYOLO(bool cuda, std::string net, std::string classfile) {};

std::vector<std::string> SearchInterface::load_class_list() {
    std::ifstream ifs("models/drone_classes.txt");
    std::string line;
    while (getline(ifs, line)) {
        class_list.push_back(line);
    }
    return class_list;
}

cv::Mat SearchInterface::format_yolov5(const cv::Mat &source) {
    int col = source.cols;
    int row = source.rows;
    int _max = MAX(col, row);
    cv::Mat result = cv::Mat::zeros(_max, _max, CV_8UC3);
    source.copyTo(result(cv::Rect(0, 0, col, row)));
    return result;
}

void SearchInterface::detect(cv::Mat &image, SearchResults &output) {
    cv::Mat blob;
    auto input_image = format_yolov5(image);
    cv::dnn::blobFromImage(input_image, blob, 1. / 255., cv::Size(settings.search_yolo_width, settings.search_yolo_height), cv::Scalar(), true, false);//settings.search_yolo_width, settings.search_yolo_height), cv::Scalar(), true, false);
    net.setInput(blob);
    std::vector<cv::Mat> outputs;
    net.forward(outputs, net.getUnconnectedOutLayersNames());
    float x_factor = input_image.cols / settings.search_yolo_width;
    float y_factor = input_image.rows / settings.search_yolo_height;

    float *data = (float *)outputs[0].data;
    std::vector<int> class_ids;
    std::vector<float> confidences;
    std::vector<cv::Rect> boxes;
    for (int i = 0; i < settings.search_dnn_model_rows; ++i)    {

        float confidence = data[4];
        if (confidence >= settings.search_confidence_threshold)        {

            float *classes_scores = data + 5;
            cv::Mat scores(1, class_list.size(), CV_32FC1, classes_scores);
            cv::Point class_id;
            double max_class_score;
            minMaxLoc(scores, 0, &max_class_score, 0, &class_id);
            if (max_class_score > settings.search_score_threshold) {
                confidences.push_back(confidence);
                
                class_ids.push_back(class_id.x);

                float x = data[0];
                float y = data[1];
                float w = data[2];
                float h = data[3];
                int left = int((x - 0.5 * w) * x_factor);
                int top = int((y - 0.5 * h) * y_factor);
                int width = int(w * x_factor);
                int height = int(h * y_factor);
                boxes.push_back(cv::Rect(left, top, width, height));
            }
        }

        data += settings.search_dnn_model_dim;
    }
    std::vector<int> nms_result;
    cv::dnn::NMSBoxes(boxes, confidences, settings.search_score_threshold, settings.search_nms_threshold, nms_result);
    for (int i = 0; i < nms_result.size(); i++)    {
        int idx = nms_result[i];
        SearchDetection result;
        result.class_id = class_ids[idx];
        result.confidence = confidences[idx];
        result.box = boxes[idx];
        output.push_back(result);
    }
}


