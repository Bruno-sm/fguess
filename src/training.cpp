#include <iostream>
#include <string>
#include <vector>
#include <set>
#include <utility>
#include <map>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <boost/filesystem.hpp>
#include "regex.hpp"
#include "file_templates.hpp"
using namespace std;
namespace fs = boost::filesystem;
#define likely(x)       __builtin_expect((x),1)
#define unlikely(x)     __builtin_expect((x),0)


string basic_pool[] = { "[a-z]", "[A-Z]", "[0-9]", "\\?" "\t", ".", "\\{", "\\}",
                        "\\n", ":", "\\<", "\\>", "#", "%", "~", "@", "=", "\\*",
                        "\\+", "\\-", "0", "1", "2", "3", "4", "5", "6", "7", "8", "9",
                        "a", "A", "b", "B", "c", "C", "d", "D", "e", "E", "f", "F", "g",
                        "G", "h", "H", "i", "I", "j", "J", "k", "K", "l", "L", "m", "M",
                        "n", "N", "o", "O", "p", "P", "q", "Q", "r", "R", "s", "S", "t",
                        "T", "u", "U", "v", "V", "w", "W", "x", "X", "y", "Y", "z", "Z"};


// Cruzamiento entre expresiones regulares
Regex crossover(const Regex &regex1, const Regex &regex2){
  int operation = (int)rand() % 4;
  switch (operation) {
    case 0: return regex1 * regex2; break; // e1e2
    case 1: return regex1 | regex2; break; // e1 + e2
    case 2: return regex1++; break; // e1+
    case 3: return *regex1; break; // e1*
  }
}


// Mutación de la expresión regular
Regex mutation(const Regex &regex, const vector<Regex> &pool){
  Regex rand_word = pool[rand() % pool.size()];
  int split_point = (int)(rand()%(regex.length()));
  return regex.head(split_point) + rand_word + regex.tail(regex.length()-split_point);
}


// Inserta n palabras escogidas aleatoriamente de los archivos en la piscina
void insertWordsFromFiles(vector<Regex> &pool, vector<ifstream> &files, int n){
  int file_index;
  int char_position, char_num;
  char character;
  string str;
  for (int i=0; i < n; i++){
    file_index = rand() % files.size();
    files[file_index].seekg(0, files[file_index].end);
    char_num = files[file_index].tellg();
    char_position = rand() % (int)(char_num/2);
    files[file_index].seekg(char_position, files[file_index].beg);
    // Nos posicionamos al principio de una palabra (Despues de uno o varios espacios)
    while (!isspace(files[file_index].peek()) && files[file_index].peek() != EOF) files[file_index].ignore();
    while (isspace(files[file_index].peek()) && files[file_index].peek() != EOF) files[file_index].ignore();
    // Leemos la palabra hasta el siguiente espacio entrecomillandola para tomar la expresion literalmente
    str = "\"";
    while (!isspace(files[file_index].peek()) && files[file_index].peek() != EOF){
      files[file_index].get(character);
      if (character == '\"')
        str += "\\";
      str.push_back(character);
    }
    str += "\"";
    pool.push_back(Regex(str));
    files[file_index].seekg(0, files[file_index].beg);
}
}


// Construye la piscina inicial
void buildInitialPool(vector<Regex> &pool, vector<ifstream> &files, int p){
  cerr << "Building initial pool" << endl;
  int basic_size = sizeof(basic_pool)/sizeof(string);
  for (int i=0; i < p; i++){
    pool.push_back(Regex(basic_pool[rand() % basic_size]));
  }
}


// Efectua el cruzamiento o la mutación de n expresiones y las introduce en la piscina
void genetic_operations(vector<Regex> &pool, int n, double epsilon){
  int r;
  for (int i=0; i < n; i++){
    r = rand() % 100;
    if (unlikely(r < epsilon*100)){
      pool.push_back(mutation(pool[rand() % pool.size()], pool));
    } else {
      pool.push_back(crossover(pool[rand() % pool.size()], pool[rand() % pool.size()]));
    }
  }
}


// Cuenta el número de coincidencias de las expresiones en los archivos
vector<int> count_matches(const vector<Regex> &regexs, const vector<fs::path> files_paths){
  CountLexTemplate countTemplate;
  for (vector<Regex>::const_iterator it = regexs.begin(); it != regexs.end(); ++it)
    countTemplate.addRegex(it->toString());
  countTemplate.save("count.lex");
  system("echo "" > out.txt");
  system("flex count.lex");
  system("gcc lex.yy.c -o count -lfl");
  fs::directory_iterator end_it;
  string command_str = "./count ";
  for (vector<fs::path>::const_iterator it = files_paths.begin(); it != files_paths.end(); ++it){
    command_str += it->string();
    command_str += " ";
  }
  command_str += "> out.txt";
  system(command_str.c_str());
  ifstream ifs("out.txt");
  vector<int> matches;
  int num;
  for (int i=0; i < regexs.size(); i++){
    ifs >> num;
    matches.push_back(num);
  }
  return matches;
}


// Cuenta el número de caracteres de los archivos
long int count_chars(vector<ifstream> &files){
  long int count = 0;

  for (vector<ifstream>::iterator it = files.begin(); it != files.end(); ++it){
    it->seekg(0, it->end);
    count += it->tellg();
    it->seekg(0, it->beg);
  }

  return count;
}


// Comparador para regex_goodness_set
struct Cmp {
  bool operator() (const pair<Regex*, double>& lpair, const pair<Regex*, double>& rpair) const{
    return lpair.second > rpair.second;
  }
};

// Selecciona las k mejores expresiones
vector<double> select_fittest(vector<Regex> &pool, int k, vector<ifstream> &current_format_files, vector<ifstream> &other_formats_files, const vector<fs::path> &current_format_file_paths, const vector<fs::path> &other_formats_file_paths){
  long int current_format_chars_count = count_chars(current_format_files);
  long int other_formats_chars_count = count_chars(other_formats_files);
  vector<int> current_format_matches = count_matches(pool, current_format_file_paths);
  vector<int> other_format_matches = count_matches(pool, other_formats_file_paths);
  set<pair<Regex*, double>, Cmp> regex_goodness_set;

  cerr << "Selecting fittest" << endl;
  double current_matches_mean, other_matches_mean;
  for (int i=0; i < pool.size(); i++){
    pair<Regex*, double> p;
    p.first = &(pool[i]);
    current_matches_mean = (long double)current_format_matches[i] / (long double)current_format_chars_count;
    other_matches_mean = (long double)other_format_matches[i] / (long double)other_formats_chars_count;
    p.second = current_matches_mean / (1000*other_matches_mean + 1);
    if (pool[i].length() > 40)
      p.second = 0;
    regex_goodness_set.insert(p);
  }

  vector<double> goodness;
  vector<Regex> new_pool;
  int i = 0;
  for (set<std::pair<Regex*, double>, Cmp>::iterator it = regex_goodness_set.begin(); i < k && it != regex_goodness_set.end(); i++, ++it){
    new_pool.push_back(*(it->first));
    goodness.push_back(it->second);
    cerr << *(it->first) << " (" << it->second << ")" << endl;
  }
  cerr << endl <<  "------------------------------------" << endl << endl;
  pool = new_pool;

  return goodness;
}


// Completa la piscina hasta el tamaño p
void complete_pool(vector<Regex> &pool, int p, double epsilon, vector<ifstream> &files){
  int free_pool_size = p - pool.size();

  // 1/3 del espacio libre se rellena con mezclas genéticas
  genetic_operations(pool, (int)(1.0/3 * free_pool_size), epsilon);

  // 1/5 del espacio libre se rellena con palabras de los archivos del formato a entrenar
  insertWordsFromFiles(pool, files, (int)(1.0/5 * free_pool_size));

  // El resto del espacio libre se rellena con elementos de la piscina inicial
  int basic_size = sizeof(basic_pool)/sizeof(string);
  for (int i=pool.size(); i < p; i++)
    pool.push_back(Regex(basic_pool[rand() % basic_size]));
}


// Entrena las expresiones para que se ajusten al formato señalado por current_format_path
void training(const fs::path &current_format_path, const fs::path &root_path, OutputTemplate &output_template, int n, int p, int k, int k_0, double epsilon){
  vector<ifstream> current_format_streams;
  vector<ifstream> other_formats_streams;
  vector<fs::path> current_format_file_paths;
  vector<fs::path> other_formats_file_paths;
  fs::directory_iterator end_it;

  for(fs::directory_iterator it(root_path/current_format_path); it != end_it; ++it)
    if (likely(fs::is_regular_file(it->status()))){
      current_format_file_paths.push_back(fs::system_complete(*it));
      current_format_streams.push_back(ifstream(fs::system_complete(*it).string()));
    }

  for(fs::directory_iterator it(root_path); it != end_it; ++it)
    if (likely(fs::is_directory(it->status())) && *it != current_format_path)
      for(fs::directory_iterator dir_it(*it); dir_it != end_it; ++dir_it)
        if (likely(fs::is_regular_file(dir_it->status()))){
          other_formats_file_paths.push_back(fs::system_complete(*dir_it));
          other_formats_streams.push_back(ifstream(fs::system_complete(*dir_it).string()));
        }


  cout << "Entrenando expresiones " << current_format_path.string() << " (0%)" << flush;

  vector<Regex> pool;
  buildInitialPool(pool, current_format_streams, p);
  for (int i=0; i < n; i++){
    complete_pool(pool, p, epsilon, current_format_streams);
    select_fittest(pool, k, current_format_streams, other_formats_streams, current_format_file_paths, other_formats_file_paths);
    cout << "\rEntrenando expresiones " << current_format_path.string() << " (" << 100*i/n << "%)" << flush;
  }
  vector<double> goodness = select_fittest(pool, k_0, current_format_streams, other_formats_streams, current_format_file_paths, other_formats_file_paths);
  cout << "\rEntrenando expresiones " << current_format_path.string() << " (100%)" << flush << endl;

  for (int i=0; i < pool.size(); i++)
    output_template.addRegex(current_format_path.string(), pool[i].toString(), goodness[i]);
}


int main(int argc, char** argv){
  int p = 50, k = 20, k_0 = 10, iter = 15;
  double epsilon = 0.01;
  fs::path examples_path(fs::initial_path<fs::path>());

  if (argc == 1){
    cout << "Especifica el directorio con los archivos de ejemplo" << endl;
    return -1;
  }
  examples_path = fs::system_complete(fs::path(argv[1]));

  for (int i=2; i < argc;)
    if (strcmp(argv[i], "-p") == 0){
      p = atoi(argv[i+1]);
      i+=2;
    } else if (strcmp(argv[i], "-k") == 0){
      k = atoi(argv[i+1]);
      i+=2;
    } else if (strcmp(argv[i], "-k_0") == 0){
      k_0 = atoi(argv[i+1]);
      i+=2;
    } else if (strcmp(argv[i], "-epsilon") == 0){
      epsilon = strtod(argv[i+1], NULL);
      i+=2;
    } else if (strcmp(argv[i], "-iter") == 0){
      iter = atoi(argv[i+1]);
      i+=2;
    } else {
      i++;
    }

  if (k > p){
    cout << "k no puede ser mayor que p" << endl;
    return -1;
  }

  if (k_0 > p){
    cout << "k_0 no puede ser mayor que p" << endl;
    return -1;
  }

  time_t t;
  srand((unsigned) time(&t));

  if (!fs::exists(examples_path)) {
    cerr << "El directorio especificado no existe" << endl;
    return -1;
  }

  vector<fs::path> example_folders;
  fs::directory_iterator end_it;
  for(fs::directory_iterator it(examples_path); it != end_it; ++it){
    if (fs::is_directory(it->status()))
      example_folders.push_back(it->path().filename());
  }

  int count;
  cout << "Muestras encontradas:" << endl;
  for (vector<fs::path>::iterator it=example_folders.begin(); it!=example_folders.end(); ++it){
    cout << it->string();
    count = 0;
    for (fs::directory_iterator dir_it(examples_path/(*it)); dir_it != end_it; ++dir_it)
      if (fs::is_regular_file(dir_it->status()))
        count++;
    cout << " [" << count << "]" << endl;
  }

  string res;
  cout << "¿Comenzar el entrenamiento? (s/n) ";
  cin >> res;
  if (res != "s" && res != "S")
    return 0;
  cout << "Empezando a entrenar:" << endl;

  OutputTemplate fguess_template;
  for (vector<fs::path>::iterator it=example_folders.begin(); it!=example_folders.end(); ++it)
    training(*it, examples_path, fguess_template, iter, p, k, k_0, epsilon);

  fguess_template.save("fguess.lex");

  return 0;
}
