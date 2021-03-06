// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// File         : the_file_io.hxx
// Author       : Pavel Aleksandrovich Koshevoy
// Created      : Tue Jul 27 10:43:00 MDT 2004
// Copyright    : (C) 2004
// License      : MIT
// Description  : File IO helper functions for common datatypes.

#ifndef THE_FILE_IO_HXX_
#define THE_FILE_IO_HXX_

// local includes:
#include "math/v3x1p3x1.hxx"
#include "utils/the_dynamic_array.hxx"
#include "utils/the_text.hxx"
#include "io/io_base.hxx"

// system includes:
#include <vector>
#include <list>
#include <utility>
#include <iostream>
#include <fstream>


extern bool save(std::ostream & stream, const char * data);
extern bool save(std::ostream & stream, const the_text_t & data);
extern bool load(std::istream & stream, the_text_t & data);

class the_registry_t;
extern bool save(std::ostream & stream, const the_registry_t & registry);
extern bool load(std::istream & stream, the_registry_t & registry);

class the_id_dispatcher_t;
extern bool save(std::ostream & stream, const the_id_dispatcher_t & d);
extern bool load(std::istream & stream, the_id_dispatcher_t & d);

class the_graph_node_ref_t;
extern bool save(std::ostream & stream, const the_graph_node_ref_t * ref);
extern bool load(std::istream & stream, the_graph_node_ref_t *& ref);

class the_graph_node_t;
extern bool save(std::ostream & stream, const the_graph_node_t * graph_node);
extern bool load(std::istream & stream, the_graph_node_t *& graph_node);


//----------------------------------------------------------------
// save
//
template <typename data_t>
bool
save(std::ostream & stream, const the_duplet_t<data_t> & duplet)
{
  return save<data_t>(stream, duplet.data(), 2);
}

//----------------------------------------------------------------
// load
//
template <typename data_t>
bool
load(std::istream & stream, the_duplet_t<data_t> & duplet)
{
  return load<data_t>(stream, duplet.data(), 2);
}


//----------------------------------------------------------------
// save
//
template <typename data_t>
bool
save(std::ostream & stream, const the_triplet_t<data_t> & triplet)
{
  return save<data_t>(stream, triplet.data(), 3);
}

//----------------------------------------------------------------
// load
//
template <typename data_t>
bool
load(std::istream & stream, the_triplet_t<data_t> & triplet)
{
  return load<data_t>(stream, triplet.data(), 3);
}


//----------------------------------------------------------------
// save
//
template <typename data_t>
bool
save(std::ostream & stream, const the_quadruplet_t<data_t> & quadruplet)
{
  return save<data_t>(stream, quadruplet.data(), 4);
}

//----------------------------------------------------------------
// load
//
template <typename data_t>
bool
load(std::istream & stream, the_quadruplet_t<data_t> & quadruplet)
{
  return load<data_t>(stream, quadruplet.data(), 4);
}


//----------------------------------------------------------------
// save
//
template <typename data_t>
bool
save(std::ostream & stream, const the_dynamic_array_t<data_t> & array)
{
  std::size_t size = array.size();
  save(stream, size);
  stream << std::endl;

  for (std::size_t i = 0; i < size; i++)
  {
    save(stream, array[i]);
    stream << std::endl;
  }

  return true;
}

//----------------------------------------------------------------
// load
//
template <typename data_t>
bool
load(std::istream & stream, the_dynamic_array_t<data_t> & array)
{
  unsigned int size = 0;
  bool ok = load(stream, size);

  for (unsigned int i = 0; i < size && ok; i++)
  {
    ok = load(stream, array[i]);
  }

  return ok;
}


//----------------------------------------------------------------
// the_loader_t
//
template <typename data_t>
class the_loader_t
{
public:
  //----------------------------------------------------------------
  // loader_fn_t
  //
  typedef bool(*loader_fn_t)(std::istream &, data_t *&);

  //----------------------------------------------------------------
  // loader_t
  //
  typedef the_loader_t<data_t> loader_t;

  the_loader_t(const the_text_t & id = the_text_t(),
	       loader_fn_t loader = NULL):
    id_(id),
    loader_(loader)
  {}

  inline bool operator == (const loader_t & l) const
  { return id_ == l.id_; }

  bool load(std::istream & istr, data_t *& data) const
  { return loader_(istr, data); }

private:
  the_text_t id_;
  loader_fn_t loader_;
};

//----------------------------------------------------------------
// the_file_io_t
//
template <typename data_t>
class the_file_io_t
{
public:
  //----------------------------------------------------------------
  // loader_t
  //
  typedef the_loader_t<data_t> loader_t;

  // add a file io handler:
  void add(const loader_t & loader)
  {
      std::size_t i = loaders_.index_of(loader);

    if (i == ~0u)
    {
      // add a new loader:
      loaders_.append(loader);
    }
    else
    {
      // replace the old loader:
      loaders_[i] = loader;
    }
  }

  // load data from a stream:
  bool load(std::istream & stream, data_t *& data) const
  {
    data = NULL;

    the_text_t magic_word;
    bool ok = ::load(stream, magic_word);
    if (!ok)
    {
      return false;
    }

    if (magic_word == "NULL")
    {
      return true;
    }

    std::cout << "loading " << magic_word << endl;
    std::size_t i = loaders_.index_of(loader_t(magic_word, NULL));
    if (i == ~0u)
    {
      return false;
    }

    const loader_t & loader = loaders_[i];
    return loader.load(stream, data);
  }

private:
  the_dynamic_array_t<loader_t> loaders_;
};

//----------------------------------------------------------------
// the_graph_node_file_io
//
extern the_file_io_t<the_graph_node_t> & the_graph_node_file_io();

//----------------------------------------------------------------
// the_graph_node_ref_file_io
//
extern the_file_io_t<the_graph_node_ref_t> & the_graph_node_ref_file_io();


//----------------------------------------------------------------
// the_loader
//
template <typename base_t, typename data_t>
bool
the_loader(std::istream & stream, base_t *& base)
{
  data_t * data = new data_t();
  bool ok = data->load(stream);
  base = data;
  return ok;
}


#endif // THE_FILE_IO_HXX_
