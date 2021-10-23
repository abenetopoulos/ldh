#if !defined(CONFIGURATION_PARSER_H)
#include "toml.hpp"

bool CheckConfiguration(void*);
void* ParseConfiguration(char*);
void* ParseAndCheckConfiguration(char*);

#define CONFIGURATION_PARSER_H
#endif
