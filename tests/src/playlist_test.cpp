/*
 * Strawberry Music Player
 * This file was part of Clementine.
 * Copyright 2010, David Sansome <me@davidsansome.com>
 * Copyright 2019, Jonas Kvinge <jonas@jkvinge.net>
 *
 * Strawberry is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Strawberry is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Strawberry.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include <memory>

#include <gtest/gtest.h>

#include "test_utils.h"

#include "collection/collectionplaylistitem.h"
#include "playlist/playlist.h"
#include "mock_settingsprovider.h"
#include "mock_playlistitem.h"

#include <QtDebug>
#include <QUndoStack>

using ::testing::Return;

// clazy:excludeall=non-pod-global-static,returning-void-expression

namespace {

class PlaylistTest : public ::testing::Test {
 protected:
  PlaylistTest()
    : playlist_(nullptr, nullptr, nullptr, 1),
      sequence_(nullptr, new DummySettingsProvider)
  {
  }

  void SetUp() override {
    playlist_.set_sequence(&sequence_);
  }

  MockPlaylistItem* MakeMockItem(const QString &title, const QString &artist = QString(), const QString &album = QString(), int length = 123) const {
    Song metadata;
    metadata.Init(title, artist, album, length);

    MockPlaylistItem *ret = new MockPlaylistItem;
    EXPECT_CALL(*ret, Metadata()).WillRepeatedly(Return(metadata));

    return ret;
  }

  PlaylistItemPtr MakeMockItemP(const QString &title, const QString &artist = QString(), const QString &album = QString(), int length = 123) const {
    return PlaylistItemPtr(MakeMockItem(title, artist, album, length));
  }

  Playlist playlist_;  // NOLINT(cppcoreguidelines-non-private-member-variables-in-classes)
  PlaylistSequence sequence_;  // NOLINT(cppcoreguidelines-non-private-member-variables-in-classes)

};

TEST_F(PlaylistTest, Basic) {
  EXPECT_EQ(0, playlist_.rowCount(QModelIndex()));
}

TEST_F(PlaylistTest, InsertItems) {

  MockPlaylistItem *item = MakeMockItem(QStringLiteral("Title"), QStringLiteral("Artist"), QStringLiteral("Album"), 123);
  PlaylistItemPtr item_ptr(item);

  // Insert the item
  EXPECT_EQ(0, playlist_.rowCount(QModelIndex()));
  playlist_.InsertItems(PlaylistItemPtrList() << item_ptr, -1);
  ASSERT_EQ(1, playlist_.rowCount(QModelIndex()));

  // Get the metadata
  EXPECT_EQ(QStringLiteral("Title"), playlist_.data(playlist_.index(0, static_cast<int>(Playlist::Column::Title))));
  EXPECT_EQ(QStringLiteral("Artist"), playlist_.data(playlist_.index(0, static_cast<int>(static_cast<int>(Playlist::Column::Artist)))));
  EXPECT_EQ(QStringLiteral("Album"), playlist_.data(playlist_.index(0, static_cast<int>(Playlist::Column::Album))));
  EXPECT_EQ(123, playlist_.data(playlist_.index(0, static_cast<int>(Playlist::Column::Length))));

}

TEST_F(PlaylistTest, Indexes) {

  playlist_.InsertItems(PlaylistItemPtrList() << MakeMockItemP(QStringLiteral("One")) << MakeMockItemP(QStringLiteral("Two")) << MakeMockItemP(QStringLiteral("Three")));
  ASSERT_EQ(3, playlist_.rowCount(QModelIndex()));

  // Start "playing" track 1
  playlist_.set_current_row(0);
  EXPECT_EQ(0, playlist_.current_row());
  EXPECT_EQ(QStringLiteral("One"), playlist_.current_item()->Metadata().title());
  EXPECT_EQ(-1, playlist_.previous_row());
  EXPECT_EQ(1, playlist_.next_row());

  // Stop playing
  EXPECT_EQ(0, playlist_.last_played_row());
  playlist_.set_current_row(-1);
  EXPECT_EQ(0, playlist_.last_played_row());
  EXPECT_EQ(-1, playlist_.current_row());

  // Play track 2
  playlist_.set_current_row(1);
  EXPECT_EQ(1, playlist_.current_row());
  EXPECT_EQ(QStringLiteral("Two"), playlist_.current_item()->Metadata().title());
  EXPECT_EQ(0, playlist_.previous_row());
  EXPECT_EQ(2, playlist_.next_row());

  // Play track 3
  playlist_.set_current_row(2);
  EXPECT_EQ(2, playlist_.current_row());
  EXPECT_EQ(QStringLiteral("Three"), playlist_.current_item()->Metadata().title());
  EXPECT_EQ(1, playlist_.previous_row());
  EXPECT_EQ(-1, playlist_.next_row());

}

TEST_F(PlaylistTest, RepeatPlaylist) {

  playlist_.InsertItems(PlaylistItemPtrList() << MakeMockItemP(QStringLiteral("One")) << MakeMockItemP(QStringLiteral("Two")) << MakeMockItemP(QStringLiteral("Three")));
  ASSERT_EQ(3, playlist_.rowCount(QModelIndex()));

  playlist_.sequence()->SetRepeatMode(PlaylistSequence::RepeatMode::Playlist);

  playlist_.set_current_row(0);
  EXPECT_EQ(1, playlist_.next_row());

  playlist_.set_current_row(1);
  EXPECT_EQ(2, playlist_.next_row());

  playlist_.set_current_row(2);
  EXPECT_EQ(0, playlist_.next_row());

}

TEST_F(PlaylistTest, RepeatTrack) {

  playlist_.InsertItems(PlaylistItemPtrList() << MakeMockItemP(QStringLiteral("One")) << MakeMockItemP(QStringLiteral("Two")) << MakeMockItemP(QStringLiteral("Three")));
  ASSERT_EQ(3, playlist_.rowCount(QModelIndex()));

  playlist_.sequence()->SetRepeatMode(PlaylistSequence::RepeatMode::Track);

  playlist_.set_current_row(0);
  EXPECT_EQ(0, playlist_.next_row());

}

TEST_F(PlaylistTest, RepeatAlbum) {

  playlist_.InsertItems(PlaylistItemPtrList()
      << MakeMockItemP(QStringLiteral("One"), QStringLiteral("Album one"))
      << MakeMockItemP(QStringLiteral("Two"), QStringLiteral("Album two"))
      << MakeMockItemP(QStringLiteral("Three"), QStringLiteral("Album one")));
  ASSERT_EQ(3, playlist_.rowCount(QModelIndex()));

  playlist_.sequence()->SetRepeatMode(PlaylistSequence::RepeatMode::Album);

  playlist_.set_current_row(0);
  EXPECT_EQ(2, playlist_.next_row());

  playlist_.set_current_row(2);
  EXPECT_EQ(0, playlist_.next_row());

}

TEST_F(PlaylistTest, RemoveBeforeCurrent) {

  playlist_.InsertItems(PlaylistItemPtrList()
      << MakeMockItemP(QStringLiteral("One")) << MakeMockItemP(QStringLiteral("Two")) << MakeMockItemP(QStringLiteral("Three")));
  ASSERT_EQ(3, playlist_.rowCount(QModelIndex()));

  // Remove a row before the currently playing track
  playlist_.set_current_row(2);
  EXPECT_EQ(2, playlist_.current_row());
  playlist_.removeRow(1, QModelIndex());
  EXPECT_EQ(1, playlist_.current_row());
  EXPECT_EQ(1, playlist_.last_played_row());
  EXPECT_EQ(0, playlist_.previous_row());
  EXPECT_EQ(-1, playlist_.next_row());

}

TEST_F(PlaylistTest, RemoveAfterCurrent) {

  playlist_.InsertItems(PlaylistItemPtrList()
      << MakeMockItemP(QStringLiteral("One")) << MakeMockItemP(QStringLiteral("Two")) << MakeMockItemP(QStringLiteral("Three")));
  ASSERT_EQ(3, playlist_.rowCount(QModelIndex()));

  // Remove a row after the currently playing track
  playlist_.set_current_row(0);
  EXPECT_EQ(0, playlist_.current_row());
  playlist_.removeRow(1, QModelIndex());
  EXPECT_EQ(0, playlist_.current_row());
  EXPECT_EQ(0, playlist_.last_played_row());
  EXPECT_EQ(-1, playlist_.previous_row());
  EXPECT_EQ(1, playlist_.next_row());

  playlist_.set_current_row(1);
  EXPECT_EQ(-1, playlist_.next_row());

}

TEST_F(PlaylistTest, RemoveCurrent) {

  playlist_.InsertItems(PlaylistItemPtrList() << MakeMockItemP(QStringLiteral("One")) << MakeMockItemP(QStringLiteral("Two")) << MakeMockItemP(QStringLiteral("Three")));
  ASSERT_EQ(3, playlist_.rowCount(QModelIndex()));

  // Remove the currently playing track's row
  playlist_.set_current_row(1);
  EXPECT_EQ(1, playlist_.current_row());
  playlist_.removeRow(1, QModelIndex());
  EXPECT_EQ(-1, playlist_.current_row());
  EXPECT_EQ(-1, playlist_.last_played_row());
  EXPECT_EQ(-1, playlist_.previous_row());
  EXPECT_EQ(0, playlist_.next_row());

}

TEST_F(PlaylistTest, InsertBeforeCurrent) {

  playlist_.InsertItems(PlaylistItemPtrList() << MakeMockItemP(QStringLiteral("One")) << MakeMockItemP(QStringLiteral("Two")) << MakeMockItemP(QStringLiteral("Three")));
  ASSERT_EQ(3, playlist_.rowCount(QModelIndex()));

  playlist_.set_current_row(1);
  EXPECT_EQ(1, playlist_.current_row());
  playlist_.InsertItems(PlaylistItemPtrList() << MakeMockItemP(QStringLiteral("Four")), 0);
  ASSERT_EQ(4, playlist_.rowCount(QModelIndex()));

  EXPECT_EQ(2, playlist_.current_row());
  EXPECT_EQ(2, playlist_.last_played_row());
  EXPECT_EQ(1, playlist_.previous_row());
  EXPECT_EQ(3, playlist_.next_row());

  EXPECT_EQ(QStringLiteral("Four"), playlist_.data(playlist_.index(0, static_cast<int>(Playlist::Column::Title))));
  EXPECT_EQ(QStringLiteral("One"), playlist_.data(playlist_.index(1, static_cast<int>(Playlist::Column::Title))));

}

TEST_F(PlaylistTest, InsertAfterCurrent) {

  playlist_.InsertItems(PlaylistItemPtrList() << MakeMockItemP(QStringLiteral("One")) << MakeMockItemP(QStringLiteral("Two")) << MakeMockItemP(QStringLiteral("Three")));
  ASSERT_EQ(3, playlist_.rowCount(QModelIndex()));

  playlist_.set_current_row(1);
  EXPECT_EQ(1, playlist_.current_row());
  playlist_.InsertItems(PlaylistItemPtrList() << MakeMockItemP(QStringLiteral("Four")), 2);
  ASSERT_EQ(4, playlist_.rowCount(QModelIndex()));

  EXPECT_EQ(1, playlist_.current_row());
  EXPECT_EQ(1, playlist_.last_played_row());
  EXPECT_EQ(0, playlist_.previous_row());
  EXPECT_EQ(2, playlist_.next_row());

  EXPECT_EQ(QStringLiteral("Two"), playlist_.data(playlist_.index(1, static_cast<int>(Playlist::Column::Title))));
  EXPECT_EQ(QStringLiteral("Four"), playlist_.data(playlist_.index(2, static_cast<int>(Playlist::Column::Title))));
  EXPECT_EQ(QStringLiteral("Three"), playlist_.data(playlist_.index(3, static_cast<int>(Playlist::Column::Title))));

}

TEST_F(PlaylistTest, Clear) {

  playlist_.InsertItems(PlaylistItemPtrList() << MakeMockItemP(QStringLiteral("One")) << MakeMockItemP(QStringLiteral("Two")) << MakeMockItemP(QStringLiteral("Three")));
  ASSERT_EQ(3, playlist_.rowCount(QModelIndex()));

  playlist_.set_current_row(1);
  EXPECT_EQ(1, playlist_.current_row());
  playlist_.Clear();

  EXPECT_EQ(0, playlist_.rowCount(QModelIndex()));
  EXPECT_EQ(-1, playlist_.current_row());
  EXPECT_EQ(-1, playlist_.last_played_row());
  EXPECT_EQ(-1, playlist_.previous_row());
  EXPECT_EQ(-1, playlist_.next_row());

}

TEST_F(PlaylistTest, UndoAdd) {

  EXPECT_FALSE(playlist_.undo_stack()->canUndo());
  EXPECT_FALSE(playlist_.undo_stack()->canRedo());

  playlist_.InsertItems(PlaylistItemPtrList() << MakeMockItemP(QStringLiteral("Title")));
  EXPECT_EQ(1, playlist_.rowCount(QModelIndex()));
  EXPECT_FALSE(playlist_.undo_stack()->canRedo());
  ASSERT_TRUE(playlist_.undo_stack()->canUndo());

  playlist_.undo_stack()->undo();
  EXPECT_EQ(0, playlist_.rowCount(QModelIndex()));
  EXPECT_FALSE(playlist_.undo_stack()->canUndo());
  ASSERT_TRUE(playlist_.undo_stack()->canRedo());

  playlist_.undo_stack()->redo();
  EXPECT_EQ(1, playlist_.rowCount(QModelIndex()));
  EXPECT_FALSE(playlist_.undo_stack()->canRedo());
  EXPECT_TRUE(playlist_.undo_stack()->canUndo());

  EXPECT_EQ(QStringLiteral("Title"), playlist_.data(playlist_.index(0, static_cast<int>(Playlist::Column::Title))));

}

TEST_F(PlaylistTest, UndoMultiAdd) {

  // Add 1 item
  playlist_.InsertItems(PlaylistItemPtrList() << MakeMockItemP(QStringLiteral("One")));

  // Add 2 items
  playlist_.InsertItems(PlaylistItemPtrList() << MakeMockItemP(QStringLiteral("Two")) << MakeMockItemP(QStringLiteral("Three")));

  // Undo adding 2 items
  ASSERT_TRUE(playlist_.undo_stack()->canUndo());
  EXPECT_EQ(QStringLiteral("add 2 songs"), playlist_.undo_stack()->undoText());
  playlist_.undo_stack()->undo();

  // Undo adding 1 item
  ASSERT_TRUE(playlist_.undo_stack()->canUndo());
  EXPECT_EQ(QStringLiteral("add 1 songs"), playlist_.undo_stack()->undoText());
  playlist_.undo_stack()->undo();

  EXPECT_FALSE(playlist_.undo_stack()->canUndo());

}

TEST_F(PlaylistTest, UndoRemove) {

  EXPECT_FALSE(playlist_.undo_stack()->canUndo());
  EXPECT_FALSE(playlist_.undo_stack()->canRedo());

  playlist_.InsertItems(PlaylistItemPtrList() << MakeMockItemP(QStringLiteral("Title")));

  EXPECT_TRUE(playlist_.undo_stack()->canUndo());
  EXPECT_FALSE(playlist_.undo_stack()->canRedo());

  playlist_.removeRow(0);

  EXPECT_EQ(0, playlist_.rowCount(QModelIndex()));
  EXPECT_FALSE(playlist_.undo_stack()->canRedo());
  ASSERT_TRUE(playlist_.undo_stack()->canUndo());

  playlist_.undo_stack()->undo();
  EXPECT_EQ(1, playlist_.rowCount(QModelIndex()));
  ASSERT_TRUE(playlist_.undo_stack()->canRedo());

  EXPECT_EQ(QStringLiteral("Title"), playlist_.data(playlist_.index(0, static_cast<int>(Playlist::Column::Title))));

  playlist_.undo_stack()->redo();
  EXPECT_EQ(0, playlist_.rowCount(QModelIndex()));
  EXPECT_FALSE(playlist_.undo_stack()->canRedo());
  EXPECT_TRUE(playlist_.undo_stack()->canUndo());

}

TEST_F(PlaylistTest, UndoMultiRemove) {

  // Add 3 items
  playlist_.InsertItems(PlaylistItemPtrList() << MakeMockItemP(QStringLiteral("One")) << MakeMockItemP(QStringLiteral("Two")) << MakeMockItemP(QStringLiteral("Three")));
  ASSERT_EQ(3, playlist_.rowCount(QModelIndex()));

  // Remove 1 item
  playlist_.removeRow(1); // Item "Two"

  // Remove 2 items
  playlist_.removeRows(0, 2); // "One" and "Three"

  ASSERT_EQ(0, playlist_.rowCount(QModelIndex()));

  // Undo removing all 3 items
  ASSERT_TRUE(playlist_.undo_stack()->canUndo());
  EXPECT_EQ(QStringLiteral("remove 3 songs"), playlist_.undo_stack()->undoText());

  playlist_.undo_stack()->undo();
  ASSERT_EQ(3, playlist_.rowCount(QModelIndex()));

}

TEST_F(PlaylistTest, UndoClear) {

  playlist_.InsertItems(PlaylistItemPtrList() << MakeMockItemP(QStringLiteral("One")) << MakeMockItemP(QStringLiteral("Two")) << MakeMockItemP(QStringLiteral("Three")));
  ASSERT_EQ(3, playlist_.rowCount(QModelIndex()));

  playlist_.Clear();
  ASSERT_EQ(0, playlist_.rowCount(QModelIndex()));
  ASSERT_TRUE(playlist_.undo_stack()->canUndo());
  EXPECT_EQ(QStringLiteral("remove 3 songs"), playlist_.undo_stack()->undoText());
  playlist_.undo_stack()->undo();

  ASSERT_EQ(3, playlist_.rowCount(QModelIndex()));

}

TEST_F(PlaylistTest, UndoRemoveCurrent) {

  playlist_.InsertItems(PlaylistItemPtrList() << MakeMockItemP(QStringLiteral("Title")));
  playlist_.set_current_row(0);
  EXPECT_EQ(0, playlist_.current_row());
  EXPECT_EQ(0, playlist_.last_played_row());

  playlist_.removeRow(0);
  EXPECT_EQ(-1, playlist_.current_row());
  EXPECT_EQ(-1, playlist_.last_played_row());

  playlist_.undo_stack()->undo();
  EXPECT_EQ(-1, playlist_.current_row());
  EXPECT_EQ(-1, playlist_.last_played_row());

}

TEST_F(PlaylistTest, UndoRemoveOldCurrent) {

  playlist_.InsertItems(PlaylistItemPtrList() << MakeMockItemP(QStringLiteral("Title")));
  playlist_.set_current_row(0);
  EXPECT_EQ(0, playlist_.current_row());
  EXPECT_EQ(0, playlist_.last_played_row());

  playlist_.removeRow(0);
  EXPECT_EQ(-1, playlist_.current_row());
  EXPECT_EQ(-1, playlist_.last_played_row());

  playlist_.set_current_row(-1);

  playlist_.undo_stack()->undo();
  EXPECT_EQ(-1, playlist_.current_row());
  EXPECT_EQ(-1, playlist_.last_played_row());

}

TEST_F(PlaylistTest, ShuffleThenNext) {

  // Add 100 items
  PlaylistItemPtrList items;
  items.reserve(100);
  for (int i=0 ; i<100 ; ++i)
    items << MakeMockItemP(QStringLiteral("Item ") + QString::number(i));
  playlist_.InsertItems(items);

  playlist_.set_current_row(0);

  // Shuffle until the current index is not at the end
  Q_FOREVER {
    playlist_.Shuffle();
    if (playlist_.current_row() != items.count()-1)
      break;
  }

  int index = playlist_.current_row();
  EXPECT_EQ(QStringLiteral("Item 0"), playlist_.current_item()->Metadata().title());
  EXPECT_EQ(QStringLiteral("Item 0"), playlist_.data(playlist_.index(index, static_cast<int>(Playlist::Column::Title))));
  EXPECT_EQ(index, playlist_.last_played_row());
  //EXPECT_EQ(index + 1, playlist_.next_row());

  // Shuffle until the current index *is* at the end
  //forever {
    //playlist_.Shuffle();
    //if (playlist_.current_row() == items.count()-1)
      //break;
  //}

  index = playlist_.current_row();
  EXPECT_EQ(QStringLiteral("Item 0"), playlist_.current_item()->Metadata().title());
  EXPECT_EQ(QStringLiteral("Item 0"), playlist_.data(playlist_.index(index, static_cast<int>(Playlist::Column::Title))));
  EXPECT_EQ(index, playlist_.last_played_row());
  //EXPECT_EQ(-1, playlist_.next_row());
  //EXPECT_EQ(index-1, playlist_.previous_row());

}

TEST_F(PlaylistTest, CollectionIdMapSingle) {

  Song song;
  song.Init(QStringLiteral("title"), QStringLiteral("artist"), QStringLiteral("album"), 123);
  song.set_id(1);

  PlaylistItemPtr item(std::make_shared<CollectionPlaylistItem>(song));
  playlist_.InsertItems(PlaylistItemPtrList() << item);

  EXPECT_EQ(0, playlist_.collection_items_by_id(-1).count());
  EXPECT_EQ(0, playlist_.collection_items_by_id(0).count());
  EXPECT_EQ(0, playlist_.collection_items_by_id(2).count());
  ASSERT_EQ(1, playlist_.collection_items_by_id(1).count());
  EXPECT_EQ(song.title(), playlist_.collection_items_by_id(1)[0]->Metadata().title());  // clazy:exclude=detaching-temporary

  playlist_.Clear();

  EXPECT_EQ(0, playlist_.collection_items_by_id(1).count());

}

TEST_F(PlaylistTest, CollectionIdMapInvalid) {

  Song invalid;
  invalid.Init(QStringLiteral("title"), QStringLiteral("artist"), QStringLiteral("album"), 123);
  ASSERT_EQ(-1, invalid.id());

  PlaylistItemPtr item(std::make_shared<CollectionPlaylistItem>(invalid));
  playlist_.InsertItems(PlaylistItemPtrList() << item);

  EXPECT_EQ(0, playlist_.collection_items_by_id(-1).count());
  EXPECT_EQ(0, playlist_.collection_items_by_id(0).count());
  EXPECT_EQ(0, playlist_.collection_items_by_id(1).count());
  EXPECT_EQ(0, playlist_.collection_items_by_id(2).count());

}

TEST_F(PlaylistTest, CollectionIdMapMulti) {

  Song one;
  one.Init(QStringLiteral("title"), QStringLiteral("artist"), QStringLiteral("album"), 123);
  one.set_id(1);

  Song two;
  two.Init(QStringLiteral("title 2"), QStringLiteral("artist 2"), QStringLiteral("album 2"), 123);
  two.set_id(2);

  PlaylistItemPtr item_one(std::make_shared<CollectionPlaylistItem>(one));
  PlaylistItemPtr item_two(std::make_shared<CollectionPlaylistItem>(two));
  PlaylistItemPtr item_three(std::make_shared<CollectionPlaylistItem>(one));
  playlist_.InsertItems(PlaylistItemPtrList() << item_one << item_two << item_three);

  EXPECT_EQ(2, playlist_.collection_items_by_id(1).count());
  EXPECT_EQ(1, playlist_.collection_items_by_id(2).count());

  playlist_.removeRow(1); // item_two
  EXPECT_EQ(2, playlist_.collection_items_by_id(1).count());
  EXPECT_EQ(0, playlist_.collection_items_by_id(2).count());

  playlist_.removeRow(1); // item_three
  EXPECT_EQ(1, playlist_.collection_items_by_id(1).count());
  EXPECT_EQ(0, playlist_.collection_items_by_id(2).count());

  playlist_.removeRow(0); // item_one
  EXPECT_EQ(0, playlist_.collection_items_by_id(1).count());
  EXPECT_EQ(0, playlist_.collection_items_by_id(2).count());

}


}  // namespace
