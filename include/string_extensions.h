#include <sstream>

template <typename T>
std::string to_string_with_precision(const T a_value, const int n = 6)
{
    std::ostringstream out;
    out.precision(n);
    out << std::fixed << a_value;
    return std::move(out).str();
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
        if( toupper(*s1) > toupper(*s2) ) return 1;
        ++s1; ++s2;
    }
    return true;
}

static inline bool ends_with(std::string const & value, std::string const & ending)
{
    if (ending.size() > value.size()) return false;
    return std::equal(ending.rbegin(), ending.rend(), value.rbegin());
}

static vector<string> explode( const string &str, const string &delimiter)
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
        if (j==delleng)//found delimiter
        {
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