#ifndef _REGEX_H_
#define _REGEX_H_

#include <string>

// Permite tratar cde forma indivisible a expresiones con más de un caracter e.g: [a-z]
class AtomicRegex{
private:
  std::string m_str;

public:

  AtomicRegex(){
    m_str = "";
  }

  AtomicRegex(const std::string &str){
    m_str = str;
  }

  std::string toString() const{
    return m_str;
  }

  friend std::ostream& operator<<(std::ostream& os, AtomicRegex& regex);
};

std::ostream& operator<<(std::ostream& os, AtomicRegex& regex){
    os << regex.toString();
}


// Expresion regular formada por una o mas expresiones atómicas
class Regex{
private:
  AtomicRegex* m_atomics;
  int m_length;

public:

  Regex(const Regex &regex){
    m_length = regex.m_length;
    m_atomics = new AtomicRegex[m_length];
    for (int i=0; i < m_length; i++)
      m_atomics[i] = regex.m_atomics[i];
  }

  Regex(){
    m_atomics = 0;
    m_length = 0;
  }

  Regex(const AtomicRegex* atomics, int n){
    m_atomics = new AtomicRegex[n];
    m_length = n;
    for (int i=0; i < n; i++)
      m_atomics[i] = atomics[i];
  }

  Regex(const std::string* strings, int n){
    m_atomics = new AtomicRegex[n];
    m_length = n;
    for (int i=0; i < n; i++)
      m_atomics[i] = AtomicRegex(strings[i]);
  }

  Regex(const std::string str){
    m_atomics = new AtomicRegex[1];
    m_length = 1;
    m_atomics[0] = AtomicRegex(str);
  }

  ~Regex(){
    delete[] m_atomics;
  }

  Regex head(int n) const{
    if (n <= 0)
      return Regex();
    if (n > m_length) n = m_length;

    return Regex(m_atomics, n);
  }

  Regex tail(int n) const{
    if (n <= 0)
      return Regex();
    if (n > m_length) n = m_length;

    AtomicRegex* atomics = new AtomicRegex[n];
    int p = m_length - n;
    for (int i=0; i < n; i++)
      atomics[i] = m_atomics[p+i];
    return Regex(atomics, n);
  }

  int length() const{
    return m_length;
  }

  // Concatena los strings de dos expresiones regulares. No tiene nada
  // que ver con la clausura ni el operador o.
  Regex operator+(const Regex& regex) const{
    AtomicRegex* atomics = new AtomicRegex[m_length + regex.m_length];
    for (int i=0; i < m_length; i++)
      atomics[i] = m_atomics[i];
    for (int i=m_length; i < m_length+regex.m_length; i++)
      atomics[i] = regex.m_atomics[i-m_length];
    return Regex(atomics, m_length + regex.m_length);
  }

  // Añade el operador | entre dos expresiones
  Regex operator|(const Regex& regex) const{
    return Regex("(") + *this + Regex("|") + regex + Regex(")");
  }

  // Concatena dos expresiones regulares
  Regex operator*(const Regex& regex) const{
    return Regex("(") + *this + regex + Regex(")");
  }

  // Clausura de la expresión regular (Añade el operador *)
  Regex operator*() const{
    return Regex("(") + *this + Regex(")") + Regex("*");
  }

  // Clausura de la expresión sin la palabra vacía (Añade el operador +)
  Regex operator++(int n) const{
    return Regex("(") + *this + Regex(")") + Regex("+");
  }

  std::string toString() const{
    std::string str = "";
    for (int i=0; i < length(); i++)
      str += m_atomics[i].toString();
    return str;
  }

  friend std::ostream& operator<<(std::ostream& os, const Regex& regex);
};

std::ostream& operator<<(std::ostream& os, const Regex& regex){
  os << regex.toString();
  return os;
}
#endif
