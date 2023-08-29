/**
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#include <unistd.h>
#include <iostream>
#include <istream>
#include <sstream>
#include <vector>
#include <cstring>
#include <fastText/fasttext.h>
#include <fasttext-wrapper.hpp>

extern "C" {

    fasttext::FastText ft_model;
    bool ft_initialized = false;

    bool ft_has_newline(std::string str) {
        return (0 == str.compare(str.length() - 1, 1, "\n"));
    };

    int ft_load_model(const char *path) {
        if (!ft_initialized) {
            if(access(path, F_OK) != 0) {
                return -1;
            }
            ft_model.loadModel(std::string(path));
            ft_initialized = true;
        }
        return 0;
    }

    // int ft_predict(const char *query_in, float *prob, char *out, int out_size) {

    //     int32_t k = 1;
    //     fasttext::real threshold = 0.0;

    //     std::string query(query_in);

    //     if(!ft_has_newline(query)) {
    //         query.append("\n");
    //     }

    //     std::istringstream inquery(query);
    //     std::istream &in = inquery;

    //     std::vector<std::pair<fasttext::real, std::string>> predictions;

    //     if(!ft_model.predictLine(in, predictions, k, threshold)) {
    //         *prob = -1;
    //         strncpy(out, "", out_size);
    //         return -1;
    //     }

    //     for(const auto &prediction : predictions) {
    //         *prob = prediction.first;
    //         strncpy(out, prediction.second.c_str(), out_size);
    //     }

    //     return 0;
    // }

    go_fast_text_pair_t* ft_predict(const char *query_in, int k, float threshold, int* result_length)
    {
        std::string query(query_in);

        if (!ft_has_newline(query)) {
            query.append("\n");
        }

        std::istringstream inquery(query);
        std::istream &in = inquery;

        std::vector<std::pair<fasttext::real, std::string>> predictions;

        ft_model.predictLine(in, predictions, k, threshold);

        int result_size = predictions.size();

        go_fast_text_pair_t* pairsArray = (go_fast_text_pair_t*) malloc(result_size * sizeof(go_fast_text_pair_t));

        for (int i = 0; i < int(predictions.size()); i++){
            const std::string::size_type label_size = predictions[i].second.size();
            pairsArray[i].label = new char[label_size + 1];
            memcpy(pairsArray[i].label, predictions[i].second.c_str(), label_size + 1);
            pairsArray[i].prob = predictions[i].first;
        }
        *result_length = result_size;

        return pairsArray;
    }

    int ft_get_vector_dimension()
    {
        if(!ft_initialized) {
            return -1;
        }
        return ft_model.getDimension();
    }

    int ft_get_sentence_vector(const char* query_in, float* vector_out, int vector_size)
    {
        std::string query(query_in);

        if(!ft_has_newline(query)) {
            query.append("\n");
        }

        std::istringstream inquery(query);
        std::istream &in = inquery;
        fasttext::Vector svec(ft_model.getDimension());

        ft_model.getSentenceVector(in, svec);
        if(svec.size() != vector_size) {
            return -1;
        }
        memcpy(vector_out, svec.data(), vector_size*sizeof(float));
        return 0;
    }

    int ft_save_model(const char* filename)
    {
        if (!ft_initialized) {
            if (access(filename, W_OK) != 0) {
                return -1;
            }
        }
        ft_model.saveModel(std::string(filename));
        return 0;
    }

    int ft_train(const char* model_name, const char* input, const char* output, int epoch, int word_ngrams, int thread, float lr)
    {
        fasttext::Args args_object;

        if (strcmp(model_name, "supervised") == 0) { 
            args_object.model = fasttext::model_name::sup;
        } else if (strcmp(model_name, "cbow") == 0) {
            args_object.model = fasttext::model_name::cbow;
        } else if (strcmp(model_name, "skipgram") == 0) {
            args_object.model = fasttext::model_name::sg;
        } else {
            return -1;
        }
            
        args_object.input = input;
        args_object.output = output;
        args_object.epoch = epoch;
        args_object.wordNgrams = word_ngrams;
        args_object.thread = thread;
        args_object.lr = lr;

        ft_model.train(args_object);
        ft_initialized = true;
        return 0;
    }
}
