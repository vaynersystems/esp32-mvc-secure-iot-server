
#include <memory>
#include <string>
#include <stdexcept>
#include <sstream>
#include <vector>
#ifndef _STRING_HELPER_H_
#define _STRING_HELPER_H_

using namespace std;

template <typename T>
std::string to_string_with_precision(const T a_value, const int n = 6)
{
    std::ostringstream out;
    out.precision(n);
    out << std::fixed << a_value;
    return std::move(out).str();
}


template<typename ... Args>
std::string string_format( const std::string& format, Args ... args )
{
    int size_s = std::snprintf( nullptr, 0, format.c_str(), args ... ) + 1; // Extra space for '\0'
    if( size_s <= 0 ){ throw std::runtime_error( "Error during formatting." ); }
    auto size = static_cast<size_t>( size_s );
    std::unique_ptr<char[]> buf( new char[ size ] );
    std::snprintf( buf.get(), size, format.c_str(), args ... );
    return std::string( buf.get(), buf.get() + size - 1 ); // We don't want the '\0' inside
}


static int compare(const char* s1, const char* s2, size_t n) {
    while( n-- != 0 ) {
        if( toupper(*s1) < toupper(*s2) ) return -1;
        if( toupper(*s1) > toupper(*s2) ) return 1;
        ++s1; ++s2;
    }
    return 0;
}

static bool iequals(const char* s1, const char* s2, size_t n) {
    while( n-- != 0 ) {
        if( toupper(*s1) < toupper(*s2) || toupper(*s1) > toupper(*s2) ) return false;        
        ++s1; ++s2;
    }
    return true;
}
static inline bool starts_with(std::string const & value, std::string const & beginning)
{
    if (beginning.size() > value.size()) return false;
    return std::equal(beginning.begin(), beginning.end(), value.begin());
}
static inline bool ends_with(std::string const & value, std::string const & ending)
{
    if (ending.size() > value.size()) return false;
    return std::equal(ending.rbegin(), ending.rend(), value.rbegin());
}

static vector<string> explode( const string &str, const string &delimiter, bool trim = false)
{
    vector<string> arr;

    int strleng = str.length();
    int delleng = delimiter.length();
    if (delleng==0)
        return arr;//no change

    int i=0;
    int k=0;
    while( i<strleng )
    {
        int j=0;
        while (i+j<strleng && j<delleng && str[i+j]==delimiter[j])
            j++;
        if(trim && i == k && str[k] == ' ') k++; //trim start

        if (j==delleng)//found delimiter
        {
            if(trim && str[i-1] == ' ') i--; //trim end
            arr.push_back(  str.substr(k, i-k) );
            i+=delleng;
            k=i;
        }
        else
        {
            i++;
        }
    }
    arr.push_back(  str.substr(k, i-k) );
    return arr;
}

static bool is_number(const std::string& s)
{
    std::string::const_iterator it = s.begin();
    while (it != s.end() && std::isdigit(*it)) ++it;
    return !s.empty() && it == s.end();
}
#endif