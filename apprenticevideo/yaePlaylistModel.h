// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created      : Wed Jul  1 20:33:02 PDT 2015
// Copyright    : Pavel Koshevoy
// License      : MIT -- http://www.opensource.org/licenses/mit-license.php

#ifndef YAE_PLAYLIST_MODEL_H_
#define YAE_PLAYLIST_MODEL_H_

// Qt includes:
#include <QAbstractItemModel>

// local includes:
#include "yaePlaylist.h"


namespace yae
{

  //----------------------------------------------------------------
  // PlaylistModel
  //
  class PlaylistModel : public QAbstractItemModel
  {
    Q_OBJECT;

    Q_PROPERTY(quint64 itemCount
               READ itemCount
               NOTIFY itemCountChanged);

  public:

    //----------------------------------------------------------------
    // Roles
    //
    enum Roles {
      kRoleType = Qt::UserRole + 1,
      kRolePath,
      kRoleLabel,
      kRoleBadge,
      kRoleGroupHash,
      kRoleItemHash,
      kRoleThumbnail,
      kRoleCollapsed,
      kRoleExcluded,
      kRoleSelected,
      kRolePlaying,
      kRoleFailed,
      kRoleItemCount
   };

    PlaylistModel(QObject * parent = NULL);

    // virtual:
    QModelIndex index(int row,
                      int column,
                      const QModelIndex & parent = QModelIndex()) const;

    // virtual:
    QModelIndex parent(const QModelIndex & child) const;

    // virtual:
    QHash<int, QByteArray> roleNames() const;

    // virtual:
    int rowCount(const QModelIndex & parent = QModelIndex()) const;
    int columnCount(const QModelIndex & parent = QModelIndex()) const;

    // virtual: returns true if parent has any children:
    bool hasChildren(const QModelIndex & parent = QModelIndex()) const;

    // virtual:
    QVariant data(const QModelIndex & index, int role) const;

    // virtual:
    bool setData(const QModelIndex & index, const QVariant & value, int role);

    // lookup a node associated with a given model index:
    PlaylistNode * getNode(const QModelIndex & index,
                           const PlaylistNode *& parentNode) const;

    // properties:
    inline quint64 itemCount() const
    { return (quint64)countItems(); }

    // popular playlist methods:
    inline const Playlist & playlist() const
    { return playlist_; }

    inline std::size_t playingItem() const
    { return playlist_.playingItem(); }

    // return number of items in the playlist:
    inline std::size_t countItems() const
    { return playlist_.countItems(); }

    // this is used to check whether previous/next navigation is possible:
    inline std::size_t countItemsAhead() const
    { return playlist_.countItemsAhead(); }

    inline std::size_t countItemsBehind() const
    { return playlist_.countItemsBehind(); }

    inline TPlaylistGroupPtr lookupGroup(std::size_t index) const
    { return playlist_.lookupGroup(index); }

    inline TPlaylistItemPtr
    lookup(std::size_t index, TPlaylistGroupPtr * group = NULL) const
    {
      return playlist_.lookup(index, group);
    }

    inline TPlaylistGroupPtr
    lookupGroup(const std::string & groupHash) const
    {
      return playlist_.lookupGroup(groupHash);
    }

    inline TPlaylistItemPtr
    lookup(const std::string & groupHash,
           const std::string & itemHash,
           std::size_t * returnItemIndex = NULL,
           TPlaylistGroupPtr * returnGroup = NULL) const
    {
      return playlist_.lookup(groupHash,
                              itemHash,
                              returnItemIndex,
                              returnGroup);
    }

    inline TPlaylistGroupPtr
    closestGroup(std::size_t itemIndex,
                 Playlist::TDirection where = Playlist::kAhead) const
    {
      return playlist_.closestGroup(itemIndex, where);
    }

    inline std::size_t
    closestItem(std::size_t itemIndex,
                Playlist::TDirection where = Playlist::kAhead,
                TPlaylistGroupPtr * group = NULL) const
    {
      return playlist_.closestItem(itemIndex, where, group);
    }

    // optionally pass back a list of hashes for the added items:
    void add(const std::list<QString> & playlist,
             std::list<BookmarkHashInfo> * returnAddedHashes = NULL);

  public slots:
    // item filter:
    bool filterChanged(const QString & filter);

    // playlist navigation controls:
    void setPlayingItem(std::size_t index);

    // selection set management:
    void selectItems(int groupRow, int itemRow, int selectionFlags);
    void selectAll();
    void unselectAll();

    void removeSelected();

  public:
    QModelIndex modelIndexForItem(std::size_t itemIndex) const;
    QModelIndex makeModelIndex(int groupRow, int itemRow) const;

    Q_INVOKABLE void setCurrentItem(int groupRow, int itemRow);
    Q_INVOKABLE void setPlayingItem(int groupRow, int itemRow);
    Q_INVOKABLE void removeItems(int groupRow, int itemRow);

  protected slots:
    void onAddingGroup(int groupRow);
    void onAddedGroup(int groupRow);

    void onAddingItem(int groupRow, int itemRow);
    void onAddedItem(int groupRow, int itemRow);

    void onRemovingGroup(int groupRow);
    void onRemovedGroup(int groupRow);

    void onRemovingItem(int groupRow, int itemRow);
    void onRemovedItem(int groupRow, int itemRow);

    void onPlayingChanged(std::size_t now, std::size_t prev);
    void onCurrentChanged(int groupRow, int itemRow);
    void onSelectedChanged(int groupRow, int itemRow);

  signals:
    void itemCountChanged();

    // this signal may be emitted if the user activates an item,
    // or otherwise changes the playlist to invalidate the
    // existing playing item:
    void playingItemChanged(std::size_t index);

    // highlight item change notification:
    void currentItemChanged(int groupRow, int itemRow);

  protected:
#if 0
    void modelIndexToRows(const QModelIndex & index,
                          int & groupRow,
                          int & itemRow) const;
#endif
    void emitDataChanged(Roles role, const QModelIndex & index);

    void emitDataChanged(Roles role,
                         const QModelIndex & first,
                         const QModelIndex & last);


    mutable Playlist playlist_;
  };
}


#endif // YAE_PLAYLIST_MODEL_H_
