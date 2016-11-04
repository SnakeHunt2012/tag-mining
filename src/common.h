#ifndef COMMON_H_
#define COMMON_H_

#include <string>
#include <vector>
#include <map>

#include <regex.h>

#include "segmenter.h"

regex_t compile_regex(const char *);
std::string regex_search(const regex_t *, int, const std::string &);
std::string regex_replace(const regex_t *, const std::string &, const std::string &);
void regex_free(regex_t *);

void load_segmenter(const char *, qss::segmenter::Segmenter **);
void segment(qss::segmenter::Segmenter *, const std::string &, std::vector<std::string> &);

#endif  // COMMON_H_
