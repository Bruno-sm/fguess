/**
 * @file regex.hpp
 * @brief Classes for the use of regular expressions
 */

#ifndef _REGEX_H_
#define _REGEX_H_

#include <string>



/**
 * @brief Allows manage indivisibly expressions with more than one character.
 */
class AtomicRegex{
private:
  std::string m_str;

public:

  /**
   * @brief Builds an empty regex.
   */
  AtomicRegex(){
    m_str = "";
  }

  /**
   * @brief Builds a regex based on a string.
   * @param str String which describes the regex.
   */
  AtomicRegex(const std::string &str){
    m_str = str;
  }

  ~AtomicRegex(){}

  /**
   * @brief Returns the regex string representation.
   */
  std::string toString() const{
    return m_str;
  }

  bool operator==(const AtomicRegex &atomic) const{
    return m_str == atomic.m_str;
  }

  friend std::ostream& operator<<(std::ostream& os, AtomicRegex& regex);
};

std::ostream& operator<<(std::ostream& os, AtomicRegex& regex){
    os << regex.toString();
}


/**
 * @brief Regular expresion composed by several AtomicRegex
 */
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

  Regex& operator=(const Regex &regex){
    m_length = regex.m_length;
    m_atomics = new AtomicRegex[m_length];
    for (int i=0; i < m_length; i++)
      m_atomics[i] = regex.m_atomics[i];
    return *this;
  }

  bool operator==(const Regex &regex) const{
    if (m_length != regex.m_length)
      return false;

    bool eq = true;
    for (int i=0; i < m_length && eq; i++)
      eq = m_atomics[i] == regex.m_atomics[i];

    return eq;
  }

  /**
   * @brief Builds an empty expression.
   */
  Regex(){
    m_atomics = 0;
    m_length = 0;
  }

  /**
   * @brief Builds the expression that concatenate all the atomics exresion.
   * @param atomics Atomic expressions which composes the whole Regex.
   * @param n Length of the atomics vector.
   */
  Regex(const AtomicRegex* atomics, int n){
    m_atomics = new AtomicRegex[n];
    m_length = n;
    for (int i=0; i < n; i++)
      m_atomics[i] = atomics[i];
  }

  /**
   * @brief Builds the expression that concatenate all the atomics exresion.
   * @param strings Atomic expressions which composes the whole Regex in its string representation.
   * @param n Length of the strings vector.
   */
  Regex(const std::string* strings, int n){
    m_atomics = new AtomicRegex[n];
    m_length = n;
    for (int i=0; i < n; i++)
      m_atomics[i] = AtomicRegex(strings[i]);
  }

  /**
   * @brief Builds a expression composed only by one atomic expression.
   * @param str Atomic expression which composes the Regex in its string representation.
   */
  Regex(const std::string str){
    m_atomics = new AtomicRegex[1];
    m_length = 1;
    m_atomics[0] = AtomicRegex(str);
  }

  ~Regex(){
    delete[] m_atomics;
  }

  /**
   * @brief Returns the Regex composed by the n first AtomicRegex of the original one.
   */
  Regex head(int n) const{
    if (n <= 0)
      return Regex();
    if (n > m_length) n = m_length;

    return Regex(m_atomics, n);
  }

  /**
   * @breif Returns the Regex composed by the n last AtomicRegex of the original one.
   */
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

  /**
   * @breif Returns the number of AtomicRegex which composes the Regex.
   */
  int length() const{
    return m_length;
  }

  /**
   * @brief Returns the Regex resulting from concatenate the AtomicRegex of two Regex.
   */
  Regex operator+(const Regex& regex) const{
    AtomicRegex* atomics = new AtomicRegex[m_length + regex.m_length];
    for (int i=0; i < m_length; i++)
      atomics[i] = m_atomics[i];
    for (int i=m_length; i < m_length+regex.m_length; i++)
      atomics[i] = regex.m_atomics[i-m_length];
    Regex concat_regex(atomics, m_length + regex.m_length);
    delete[] atomics;
    return concat_regex;
  }

  /**
   * @brief Returns the Regex resulting from add the | (or) operator between two expressions.
   */
  Regex operator|(const Regex& regex) const{
    return Regex("(") + *this + Regex("|") + regex + Regex(")");
  }

  /**
   * @brief Returns the Regex resulting from the concatenation operation between two regular expressions.
   */
  Regex operator*(const Regex& regex) const{
    return Regex("(") + *this + regex + Regex(")");
  }

  /**
   * @brief Returns the Regex with the * operator (clausure) to the Regex.
   */
  Regex operator*() const{
    return Regex("(") + *this + Regex(")") + Regex("*");
  }

  /**
   * @brief Returns the Regex with the + operator (clausure without empty word) to the Regex.
   */
  Regex operator++(int n) const{
    return Regex("(") + *this + Regex(")") + Regex("+");
  }

  /**
   * @brief Returns the Regex string representation.
   */
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
