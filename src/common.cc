#include <stdexcept>
#include <fstream>
#include <sstream>

#include <stdlib.h>

#include "config.h"
#include "common.h"

using namespace std;

regex_t compile_regex(const char *pattern)
{
    regex_t regex;
    int error_code = regcomp(&regex, pattern, REG_EXTENDED);
    if (error_code != 0) {
        size_t length = regerror(error_code, &regex, NULL, 0);
        char *buffer = (char *) malloc(sizeof(char) * length);
        (void) regerror(error_code, &regex, buffer, length);
        string error_message = string(buffer);
        free(buffer);
        throw runtime_error(string("error: unable to compile regex '") + pattern + "', message: " + error_message);
    }
    return regex;
}

string regex_search(const regex_t *regex, int field, const string &line)
{
    regmatch_t match_res[field + 1];
    int error_code = regexec(regex, line.c_str(), field + 1, match_res, 0);
    if (error_code != 0) {
        size_t length = regerror(error_code, regex, NULL, 0);
        char *buffer = (char *) malloc(sizeof(char) * length);
        (void) regerror(error_code, regex, buffer, length);
        string error_message = string(buffer);
        free(buffer);
        throw runtime_error(string("error: unable to execute regex, message: ") + error_message);
    }
    return string(line, match_res[field].rm_so, match_res[field].rm_eo - match_res[field].rm_so);
}

string regex_replace(const regex_t *regex, const string &sub_string, const string &ori_string)
{
    regmatch_t match_res;
    int error_code;
    string res_string = ori_string;
    while ((error_code = regexec(regex, res_string.c_str(), 1, &match_res, 0)) != REG_NOMATCH) {
        if (error_code != 0) {
            size_t length = regerror(error_code, regex, NULL, 0);
            char *buffer = (char *) malloc(sizeof(char) * length);
            (void) regerror(error_code, regex, buffer, length);
            string error_message = string(buffer);
            free(buffer);
            throw runtime_error(string("error: unable to execute regex, message: ") + error_message);
        }
        res_string = string(res_string, 0, match_res.rm_so) + sub_string + string(res_string, match_res.rm_eo, res_string.size() - match_res.rm_eo);
    }
    return res_string;
}

void regex_free(regex_t *regex)
{
    regfree(regex);
}

void load_segmenter(const char *conf_file, qss::segmenter::Segmenter **segmenter)
{
    qss::segmenter::Config::get_instance()->init(conf_file);
    *segmenter = qss::segmenter::CreateSegmenter();
    if (!segmenter)
        throw runtime_error("error: loading segmenter failed");
}

void segment(qss::segmenter::Segmenter *segmenter, const string &line, vector<string> &seg_res)
{
    int buffer_size = line.size() * 2;
    char *buffer = (char *) malloc(sizeof(char) * buffer_size);
    int res_size = segmenter->segmentUtf8(line.c_str(), line.size(), buffer, buffer_size);
    
    stringstream ss(string(buffer, res_size));
    for (string token; ss >> token; seg_res.push_back(token)) ;
    
    free(buffer);
}

