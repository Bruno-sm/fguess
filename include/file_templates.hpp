 /**
 * @file file_templates.hpp
 * @brief Classes for generate the lex files count.lex and fguess.lex
 */

#ifndef _FILE_TEMPLATES_H_
#define _FILE_TEMPLTES_H_
#include <fstream>
#include <string>
#include <stdlib.h>



/**
 * @brief Parent class for the two kinds of templates.
 */
class FileTemplate{
public:

  /**
   * @brief Transform the internal state of the template in a lex program and
   * returns it in a string.
   */
  virtual std::string toString() = 0;

  /**
   * @brief Writes the program in a file.
   * @param filename Name of the output file.
   */
  void save(const std::string &filename){
    std::ofstream of(filename);
    of << toString();
  }
};


/**
 * @brief Generates a lex program to count the number of regexs matches in several files.
 */
class CountLexTemplate : public FileTemplate{
private:
  std::vector<std::string> regexs;

public:

  /**
   * @brief Stores the regex in order to count its matches in the generated program.
   */
  void addRegex(const std::string& regex){
    regexs.push_back(regex);
  }

  /**
   * @brief Generates a lex program which count the matches in several files for all the stored regex and
   * prints the values separated by an space.
   */
  std::string toString(){
    std::string rules = "";
    for (int i=0; i < regexs.size(); i++){
      rules += regexs[i];
      rules += " {regex_count[";
      rules += std::to_string(i);
      rules += "]++; REJECT;}\n";
    }

    std::string str = "";
    str +=
    " /*----Declarations section----*/\n"
    "%{\n"
    "#include <stdio.h>\n"
    "#define regex_num ";
    str += std::to_string(regexs.size());
    str +=
    "\n"
    "int regex_count[regex_num];\n"
    "%}\n\n"
    "%%\n"
    " /*----Rules section----*/\n";
    str  += rules;
    str +=
    ".|\\n {;}"
    "\n"
    "%%\n"
    " /*----Procedures section----*/\n"
    "int main(int argc, char** argv){\n"
    " if (argc < 2){\n"
    "   printf(\"Insert the input files\\n\");\n"
    "   return -1;\n"
    " }\n"
    " for (int i=0; i < regex_num; i++)\n"
    "   regex_count[i] = 0;\n"
    " for (int i=1; i < argc; i++){\n"
    "   yyin = fopen(argv[i], \"rt\");\n"
    "   if(yyin == NULL){\n"
    "     printf(\"The file %s can't be opened\\n\", argv[1]);\n"
    "     exit(-1);\n"
    "   }\n"
    "   yylex();\n"
    " }\n"
    " for (int i=0; i < regex_num; i++)\n"
    "   printf(\"%d \", regex_count[i]);\n"
    " return 0;\n"
    "}";

    return str;
  }
};


/**
 * @brief Generates the fguess.lex program which calculates the reliability with which a file is of a certain format.
 */
class OutputTemplate : public FileTemplate{
private:
  std::map<std::string, std::vector<std::pair<std::string, double> > > regex_data;

public:

  /**
   * @brief Stores a regex which should be trained to fit with a format alongside its goodness.
   * @param format_name Format for which the regex has been trained to fit.
   * @param regex Regex to store.
   * @param goodness Goodness of the regex.
   */
  void addRegex(const std::string &format_name, const std::string &regex, double goodness){
    regex_data[format_name].push_back(std::pair<std::string, double>(regex, goodness));
  }


  /**
   * @brief Generates a lex program which calculates the reliability with which a file is of a certain format
   * using the expressions and goodness stored.
   */
  std::string toString(){
    std::string rules = "";
    std::string init_regex_data = "";
    int i = 0, j = 0;
    std::map<std::string, std::vector<std::pair<std::string, double> > >::iterator it;
    for (it = regex_data.begin(); it != regex_data.end(); ++it, i++){
      j = 0;
      for (std::vector<std::pair<std::string, double> >::iterator it2 = (*it).second.begin(); it2 != (*it).second.end(); ++it2, j++){
        rules += it2->first;
        rules += " {formats_regex[";
        rules += std::to_string(i);
        rules += "][";
        rules += std::to_string(j);
        rules += "].matches++; REJECT;}\n";

        init_regex_data += "  initRegexData(&formats_regex[";
        init_regex_data += std::to_string(i);
        init_regex_data += "][";
        init_regex_data += std::to_string(j);
        init_regex_data += "], ";
        init_regex_data += std::to_string(it2->second);
        init_regex_data += ");\n";
      }
    }

    std::string regex_num = "{";
    std::string format_names = "{";
    for (it = regex_data.begin(); it != regex_data.end(); ++it){
      regex_num += std::to_string(it->second.size()) + ",";
      format_names += "\"" + it->first + "\"" + ",";
    }
    regex_num.pop_back();
    regex_num += "}";
    format_names.pop_back();
    format_names += "}";

    std::string str = "";
    str +=
  "  /*----Declarations section----*/\n"
  "%{"
  "#include <stdio.h>\n"

  "struct RegexData{\n"
  "  double goodness;\n"
  "  int matches;\n"
  "};\n"

  "void initRegexData(struct RegexData* rd, double goodness){\n"
  "  rd->goodness = goodness;\n"
  "  rd->matches = 0;\n"
  "}\n"

  "#define formats_num ";
  str += std::to_string(regex_data.size());
  str +=
  " // Number of formats for which we obtain the reliability\n"
  "struct RegexData** formats_regex;\n"
  "%}\n"
  "\n"
  "%%\n"
  "  /*----Rules section----*/\n";
  str += rules;
  str +=
  ".|\\n {;}\n"
  "\n"
  "%%\n"
  "  /*----Procedures section----*/\n"
  "int main(int argc, char** argv){\n"
  "  if (argc == 2){\n"
  "    yyin = fopen(argv[1], \"rt\");\n"
  "    if(yyin == NULL){\n"
  "      printf(\"The file %s can't be opened\\n\", argv[1]);\n"
  "      exit(-1);\n"
  "    }\n"
  "  }\n"
  "  else yyin = stdin;\n"
  "  char* format_names[] = ";
  str += format_names;
  str += ";\n"
  "  formats_regex = (struct RegexData**)malloc(sizeof(struct RegexData*)*formats_num);\n"
  "  int regex_num[] = ";
  str += regex_num;
  str += ";\n"
  "  for (int i=0; i < formats_num; i++)\n"
  "    formats_regex[i] = (struct RegexData*)malloc(sizeof(struct RegexData)*regex_num[i]);\n";
  str += init_regex_data;
  str +=
  "\n"

  "  yylex();\n"

  "  double reliability;\n"
  "  for (int i=0; i < formats_num; i++){\n"
  "    reliability = 0;\n"
  "    for (int j=0; j < regex_num[i]; j++){\n"
  "      reliability += formats_regex[i][j].goodness * formats_regex[i][j].matches;\n"
  "    }\n"
  "    reliability = reliability / (reliability+100);\n"
  "    printf(\"The file is %s whith a reliability of %lf\\n\", format_names[i], reliability);\n"
  "  }\n"

  "  for (int i=0; i < formats_num; i++)\n"
  "    free(formats_regex[i]);\n"
  "  free(formats_regex);\n"
  "  return 0;\n"
  "}";

  return str;
  }
};

#endif
