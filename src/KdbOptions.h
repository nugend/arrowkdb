#ifndef __KDB_OPTIONS__
#define __KDB_OPTIONS__

#include <string>
#include <map>
#include <stdexcept>
#include <cctype>
#include <set>

#include "k.h"


namespace kx {
namespace arrowkdb {

// Supported options
namespace Options
{
  // Int options
  const std::string PARQUET_CHUNK_SIZE = "PARQUET_CHUNK_SIZE";
  const std::string PARQUET_MULTITHREADED_READ = "PARQUET_MULTITHREADED_READ";
  const std::string USE_MMAP = "USE_MMAP";
  const std::string DECIMAL128_AS_DOUBLE = "DECIMAL128_AS_DOUBLE";

  // String options
  const std::string PARQUET_VERSION = "PARQUET_VERSION";

  // Dict options
  const std::string NULL_MAPPING = "NULL_MAPPING";

  // Null mapping options
  const std::string NM_INT_16 = "int16";
  const std::string NM_INT_32 = "int32";

  const static std::set<std::string> int_options = {
    PARQUET_CHUNK_SIZE,
    PARQUET_MULTITHREADED_READ,
    USE_MMAP,
    DECIMAL128_AS_DOUBLE,
  };
  const static std::set<std::string> string_options = {
    PARQUET_VERSION,
  };
  const static std::set<std::string> dict_options = {
    NULL_MAPPING,
  };
  const static std::set<std::string> null_mapping_options = {
    NM_INT_16,
    NM_INT_32,
  };
}


// Helper class for reading dictionary of options
//
// Dictionary key:    KS
// Dictionary value:  KS or
//                    KJ or
//                    XD or
//                    0 of -KS|-KJ|XD|KC
class KdbOptions
{
private:
  std::map<std::string, std::string> null_mapping_options;
  std::map<std::string, std::string> string_options;
  std::map<std::string, int64_t> int_options;

  const std::set<std::string>& supported_string_options;
  const std::set<std::string>& supported_int_options;
  const std::set<std::string>& supported_dict_options;
  const std::set<std::string>& supported_null_mapping_options;

private:
  const std::string ToUpper(std::string str) const
  {
    std::string upper;
    for (auto i : str)
      upper.push_back((unsigned char)std::toupper(i));
    return upper;
  }

  void PopulateIntOptions(K keys, K values)
  {
    for (auto i = 0ll; i < values->n; ++i) {
      const std::string key = ToUpper(kS(keys)[i]);
      if (supported_int_options.find(key) == supported_int_options.end())
        throw InvalidOption(("Unsupported int option '" + key + "'").c_str());
      int_options[key] = kJ(values)[i];
    }
  }

  void PopulateStringOptions(K keys, K values)
  {
    for (auto i = 0ll; i < values->n; ++i) {
      const std::string key = ToUpper(kS(keys)[i]);
      if (supported_string_options.find(key) == supported_string_options.end())
        throw InvalidOption(("Unsupported string option '" + key + "'").c_str());
      string_options[key] = ToUpper(kS(values)[i]);
    }
  }

  void PopulateDictOptions( K keys, K values )
  {
    for( auto i = 0ll; i < values->n; ++i ) {
      const std::string key = ToUpper( kS( keys )[i] );
      if( supported_dict_options.find( key ) == supported_dict_options.end() ){
        throw InvalidOption(("Unsupported dict option '" + key + "'").c_str());
      }

      K dict = kK( values )[0];
      K options = kK( values )[1];
      for( auto j = 0ll; j < options->n; ++j ) {
        const std::string option = ToUpper( kS( dict )[j] );
        if( supported_null_mapping_options.find( key ) == supported_null_mapping_options.end() ){
            throw InvalidOption(("Unsupported '" + key + "' option '" + option + "'").c_str());
        }
        null_mapping_options[option] = ToUpper( kS( options )[j] );
      }
    }
  }

  void PopulateMixedOptions(K keys, K values)
  {
    for (auto i = 0ll; i < values->n; ++i) {
      const std::string key = ToUpper(kS(keys)[i]);
      K value = kK(values)[i];
      switch (value->t) {
      case -KJ:
        if (supported_int_options.find(key) == supported_int_options.end())
          throw InvalidOption(("Unsupported int option '" + key + "'").c_str());
        int_options[key] = value->j;
        break;
      case -KS:
        if (supported_string_options.find(key) == supported_string_options.end())
          throw InvalidOption(("Unsupported string option '" + key + "'").c_str());
        string_options[key] = ToUpper(value->s);
        break;
      case KC:
      {
        if (supported_string_options.find(key) == supported_string_options.end())
          throw InvalidOption(("Unsupported string option '" + key + "'").c_str());
        string_options[key] = ToUpper(std::string((char*)kG(value), value->n));
        break;
      }
      case XD:
      {
        if( supported_dict_options.find( key ) == supported_dict_options.end() ){
          throw InvalidOption(("Unsupported dict option '" + key + "'").c_str());
        }
        K dict = kK( values )[0];
        K options = kK( values )[1];
        for( auto j = 0ll; j < options->n; ++j ) {
          const std::string option = ToUpper( kS( dict )[j] );
          if( supported_null_mapping_options.find( key ) == supported_null_mapping_options.end() ){
              throw InvalidOption(("Unsupported '" + key + "' option '" + option + "'").c_str());
          }
          null_mapping_options[option] = ToUpper( kS( options )[j] );
        }
        break;
      }
      case 101:
        // Ignore ::
        break;
      default:
        throw InvalidOption(("option '" + key + "' value not -7|-11|10h").c_str());
      }
    }
  }

public:
  class InvalidOption : public std::invalid_argument
  {
  public:
    InvalidOption(const std::string message) : std::invalid_argument(message.c_str())
    {};
  };

  KdbOptions(
          K options
        , const std::set<std::string> supported_string_options_
        , const std::set<std::string> supported_int_options_
        , const std::set<std::string>& supported_dict_options_ = std::set<std::string> {}
        , const std::set<std::string>& supported_null_mapping_options_ = std::set<std::string> {} )
    : supported_string_options(supported_string_options_)
    , supported_int_options(supported_int_options_)
    , supported_dict_options( supported_dict_options_ )
    , supported_null_mapping_options( supported_null_mapping_options_ )
  {
    if (options != NULL && options->t != 101) {
      if (options->t != 99)
        throw InvalidOption("options not -99h");
      K keys = kK(options)[0];
      if (keys->t != KS)
        throw InvalidOption("options keys not 11h");
      K values = kK(options)[1];
      switch (values->t) {
      case KJ:
        PopulateIntOptions(keys, values);
        break;
      case KS:
        PopulateStringOptions(keys, values);
        break;
      case XD:
        PopulateDictOptions(keys, values);
        break;
      case 0:
        PopulateMixedOptions(keys, values);
        break;
      default:
        throw InvalidOption("options values not 7|11|0h");
      }
    }
  }

  bool GetNullMappingOption( const std::string key, std::string& result ) const
  {
      const auto it = null_mapping_options.find( key );
      if( it == null_mapping_options.end() )
      {
          return false;
      }
      else
      {
          result = it->second;
          return true;
      }
  }

  bool GetStringOption(const std::string key, std::string& result) const
  {
    const auto it = string_options.find(key);
    if (it == string_options.end())
      return false;
    else {
      result = it->second;
      return true;
    }
  }

  bool GetIntOption(const std::string key, int64_t& result) const
  {
    const auto it = int_options.find(key);
    if (it == int_options.end())
      return false;
    else {
      result = it->second;
      return true;
    }
  }
};

} // namespace arrowkdb
} // namespace kx


#endif // __KDB_OPTIONS__
