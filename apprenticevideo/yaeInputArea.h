// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created      : Tue Oct 20 19:19:59 PDT 2015
// Copyright    : Pavel Koshevoy
// License      : MIT -- http://www.opensource.org/licenses/mit-license.php

#ifndef YAE_INPUT_AREA_H_
#define YAE_INPUT_AREA_H_

// standard libraries:
#include <list>
#include <stdexcept>

// boost includes:
#ifndef Q_MOC_RUN
#include <boost/shared_ptr.hpp>
#endif

// Qt interfaces:
#include <QPersistentModelIndex>

// local interfaces:
#include "yaeItem.h"
#include "yaeVec.h"


namespace yae
{

  //----------------------------------------------------------------
  // InputArea
  //
  struct InputArea : public Item
  {
    InputArea(const char * id);

    // virtual:
    void getInputHandlers(// coordinate system origin of
                          // the input area, expressed in the
                          // coordinate system of the root item:
                          const TVec2D & itemCSysOrigin,

                          // point expressed in the coord. system of the item,
                          // rootCSysPoint = itemCSysOrigin + itemCSysPoint
                          const TVec2D & itemCSysPoint,

                          // pass back input areas overlapping above point,
                          // along with its coord. system origin expressed
                          // in the coordinate system of the root item:
                          std::list<InputHandler> & inputHandlers);

    // NOTE: default implementation will simply call the onXxx_ delegate
    // if one is provided, otherwise it will return false:

    virtual void onCancel();

    virtual bool onMouseOver(const TVec2D & itemCSysOrigin,
                             const TVec2D & rootCSysPoint);

    virtual bool onScroll(const TVec2D & itemCSysOrigin,
                          const TVec2D & rootCSysPoint,
                          double degrees);

    virtual bool onPress(const TVec2D & itemCSysOrigin,
                         const TVec2D & rootCSysPoint);

    virtual bool onClick(const TVec2D & itemCSysOrigin,
                         const TVec2D & rootCSysPoint);

    virtual bool onDoubleClick(const TVec2D & itemCSysOrigin,
                               const TVec2D & rootCSysPoint);

    virtual bool onDrag(const TVec2D & itemCSysOrigin,
                        const TVec2D & rootCSysDragStart,
                        const TVec2D & rootCSysDragEnd);

    // NOTE: default implementation of onDragEnd will call onDragEnd_
    // if one is provided, otherwise it will call onDrag(...):
    virtual bool onDragEnd(const TVec2D & itemCSysOrigin,
                           const TVec2D & rootCSysDragStart,
                           const TVec2D & rootCSysDragEnd);

    struct OnCancel
    {
      virtual ~OnCancel() {}
      virtual void process(Item & inputAreaParent) = 0;
    };

    struct OnScroll
    {
      virtual ~OnScroll() {}
      virtual bool process(Item & inputAreaParent,
                           const TVec2D & itemCSysOrigin,
                           const TVec2D & rootCSysPoint,
                           double degrees) = 0;
    };

    struct OnInput
    {
      virtual ~OnInput() {}
      virtual bool process(Item & inputAreaParent,
                           const TVec2D & itemCSysOrigin,
                           const TVec2D & rootCSysPoint) = 0;
    };

    struct OnDrag
    {
      virtual ~OnDrag() {}
      virtual bool process(Item & inputAreaParent,
                           const TVec2D & itemCSysOrigin,
                           const TVec2D & rootCSysDragStart,
                           const TVec2D & rootCSysDragEnd) = 0;
    };

    typedef boost::shared_ptr<OnCancel> TOnCancel;
    typedef boost::shared_ptr<OnScroll> TOnScroll;
    typedef boost::shared_ptr<OnInput> TOnInput;
    typedef boost::shared_ptr<OnDrag> TOnDrag;

    // one does not have to subclass the InputArea to override
    // default behavior -- simply provide a delegate for
    // the behavior that should be customized:
    TOnCancel onCancel_;
    TOnInput onMouseOver_;
    TOnScroll onScroll_;
    TOnInput onPress_;
    TOnInput onClick_;
    TOnInput onDoubleClick_;
    TOnDrag onDrag_;
    TOnDrag onDragEnd_;
  };

  //----------------------------------------------------------------
  // ModelInputArea
  //
  template <typename Model>
  struct ModelInputArea : public InputArea
  {
    typedef Model TModel;
    typedef ModelItem<Model> TModelItem;

    ModelInputArea(const char * id):
      InputArea(id)
    {}

    // lookup the closest ancestor model item associated with this input area:
    TModelItem & lookupModelItem() const
    {
      boost::shared_ptr<TModelItem> modelItem = modelItem_.lock();
      if (!modelItem)
      {
        TModelItem * found = this->hasAncestor<TModelItem>();

        if (!found)
        {
          YAE_ASSERT(false);
          throw std::runtime_error("ModelInputArea requires "
                                   "ModelItem ancestor");
        }

        // update weak reference:
        modelItem = found->template sharedPtr<TModelItem>();
        modelItem_ = modelItem;
      }

      return *modelItem;
    }

    inline Model & model() const
    { return lookupModelItem().model(); }

    inline const QPersistentModelIndex & modelIndex() const
    { return lookupModelItem().modelIndex(); }

  protected:
    // cached model item associated with this input area:
    mutable boost::weak_ptr<TModelItem> modelItem_;
  };


  //----------------------------------------------------------------
  // ClickableItem
  //
  template <typename Model>
  struct ClickableItem : public ModelInputArea<Model>
  {
    ClickableItem(const char * id):
      ModelInputArea<Model>(id)
    {}

    // virtual:
    bool onPress(const TVec2D & itemCSysOrigin,
                 const TVec2D & rootCSysPoint)
    { return true; }
  };

  //----------------------------------------------------------------
  // MouseTrap
  //
  // Consume mouse events to prevent them from
  // propagating to a lower level:
  //
  struct MouseTrap : public InputArea
  {
    MouseTrap(const char * id):
      InputArea(id)
    {}

    // virtual:
    bool onScroll(const TVec2D & itemCSysOrigin,
                  const TVec2D & rootCSysPoint,
                  double degrees)
    { return true; }

    // virtual:
    bool onPress(const TVec2D & itemCSysOrigin,
                 const TVec2D & rootCSysPoint)
    { return true; }

    // virtual:
    bool onClick(const TVec2D & itemCSysOrigin,
                 const TVec2D & rootCSysPoint)
    { return true; }

    // virtual:
    bool onDoubleClick(const TVec2D & itemCSysOrigin,
                       const TVec2D & rootCSysPoint)
    { return true; }

    // virtual:
    bool onDrag(const TVec2D & itemCSysOrigin,
                const TVec2D & rootCSysDragStart,
                const TVec2D & rootCSysDragEnd)
    { return true; }
  };

}


#endif // YAE_INPUT_AREA_H_