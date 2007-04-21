// NOTE: 
//       DO NOT EDIT THIS FILE, IT IS AUTOGENERATED.
//       ALL CHANGES WILL BE LOST UPON REGENERATION.

#ifndef THE_POINT_SYMBOLS_HXX_
#define THE_POINT_SYMBOLS_HXX_

// local includes:
#include "opengl/the_symbols.hxx"


//----------------------------------------------------------------
// the_point_symbol_id_t
// 
typedef enum
{
  THE_CIRCLE_SYMBOL_E,
  THE_CORNERS_SYMBOL_E,
  THE_CROSS_SYMBOL_E,
  THE_DASH_CROSS_SYMBOL_E,
  THE_DIAG_CROSS_SYMBOL_E,
  THE_DIAG_SQUARE_SYMBOL_E,
  THE_FILLED_CIRCLE_SYMBOL_E,
  THE_FILLED_DIAG_SQUARE_SYMBOL_E,
  THE_FILLED_SQUARE_SYMBOL_E,
  THE_HASH_SYMBOL_E,
  THE_SMALL_CIRCLE_SYMBOL_E,
  THE_SMALL_CORNERS_SYMBOL_E,
  THE_SMALLEST_FILLED_DIAG_SQUARE_SYMBOL_E,
  THE_SMALL_FILLED_CIRCLE_SYMBOL_E,
  THE_SMALL_FILLED_DIAG_SQUARE_SYMBOL_E,
  THE_SMALL_FILLED_SQUARE_SYMBOL_E,
  THE_SMALL_SQUARE_SYMBOL_E,
  THE_SQUARE_SYMBOL_E,
  THE_TRIANGLE_SYMBOL_E,
  THE_TRI_AXIS_SYMBOL_E
} the_point_symbol_id_t;


//----------------------------------------------------------------
// the_point_symbols_t
// 
class the_point_symbols_t : public the_symbols_t
{
public:
  the_point_symbols_t(): the_symbols_t("point") {}
  virtual ~the_point_symbols_t() {}

  // virtual:
  unsigned int width() const  { return 8; }
  unsigned int height() const { return 8; }

  // virtual:
  float x_origin() const { return 4.0; }
  float y_origin() const { return 4.0; }

  // virtual:
  float x_step() const { return 0.0; }
  float y_step() const { return 0.0; }

  // virtual:
  unsigned char * bitmap(const unsigned int & id) const
  { return symbols_[id]; }

  // virtual: the number of display lists to be compiled:
  unsigned int size() const { return 20; }

private:
  static unsigned char symbols_[20][8];
};

//----------------------------------------------------------------
// THE_POINT_SYMBOLS
// 
// a single global instance of the symbols font:
// 
extern the_point_symbols_t THE_POINT_SYMBOLS;


#endif // THE_POINT_SYMBOLS_HXX_
