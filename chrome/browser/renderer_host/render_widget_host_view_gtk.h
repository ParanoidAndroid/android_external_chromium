// Copyright (c) 2010 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_RENDERER_HOST_RENDER_WIDGET_HOST_VIEW_GTK_H_
#define CHROME_BROWSER_RENDERER_HOST_RENDER_WIDGET_HOST_VIEW_GTK_H_

#include <gdk/gdk.h>

#include <map>
#include <vector>
#include <string>

#include "base/scoped_ptr.h"
#include "base/time.h"
#include "chrome/browser/renderer_host/render_widget_host_view.h"
#include "chrome/common/owned_widget_gtk.h"
#include "gfx/native_widget_types.h"
#include "webkit/glue/plugins/gtk_plugin_container_manager.h"
#include "webkit/glue/webcursor.h"

class RenderWidgetHost;
class GpuViewHost;
class GtkIMContextWrapper;
class GtkKeyBindingsHandler;
class MenuGtk;
struct NativeWebKeyboardEvent;

#if defined(OS_CHROMEOS)
namespace views {
class TooltipWindowGtk;
}
#endif  // defined(OS_CHROMEOS)

typedef struct _GtkClipboard GtkClipboard;
typedef struct _GtkSelectionData GtkSelectionData;

// -----------------------------------------------------------------------------
// See comments in render_widget_host_view.h about this class and its members.
// -----------------------------------------------------------------------------
class RenderWidgetHostViewGtk : public RenderWidgetHostView {
 public:
  explicit RenderWidgetHostViewGtk(RenderWidgetHost* widget);
  ~RenderWidgetHostViewGtk();

  // Initialize this object for use as a drawing area.
  void InitAsChild();

  // RenderWidgetHostView implementation.
  virtual void InitAsPopup(RenderWidgetHostView* parent_host_view,
                           const gfx::Rect& pos);
  virtual RenderWidgetHost* GetRenderWidgetHost() const { return host_; }
  virtual void DidBecomeSelected();
  virtual void WasHidden();
  virtual void SetSize(const gfx::Size& size);
  virtual gfx::NativeView GetNativeView();
  virtual void MovePluginWindows(
      const std::vector<webkit_glue::WebPluginGeometry>& moves);
  virtual void Focus();
  virtual void Blur();
  virtual bool HasFocus();
  virtual void Show();
  virtual void Hide();
  virtual bool IsShowing();
  virtual gfx::Rect GetViewBounds() const;
  virtual void UpdateCursor(const WebCursor& cursor);
  virtual void SetIsLoading(bool is_loading);
  virtual void ImeUpdateTextInputState(WebKit::WebTextInputType type,
                                       const gfx::Rect& caret_rect);
  virtual void ImeCancelComposition();
  virtual void DidUpdateBackingStore(
      const gfx::Rect& scroll_rect, int scroll_dx, int scroll_dy,
      const std::vector<gfx::Rect>& copy_rects);
  virtual void RenderViewGone();
  virtual void Destroy();
  virtual void WillDestroyRenderWidget(RenderWidgetHost* rwh) {}
  virtual void SetTooltipText(const std::wstring& tooltip_text);
  virtual void SelectionChanged(const std::string& text);
  virtual void ShowingContextMenu(bool showing);
  virtual BackingStore* AllocBackingStore(const gfx::Size& size);
  virtual VideoLayer* AllocVideoLayer(const gfx::Size& size);
  virtual void SetBackground(const SkBitmap& background);
  virtual void CreatePluginContainer(gfx::PluginWindowHandle id);
  virtual void DestroyPluginContainer(gfx::PluginWindowHandle id);
  virtual void SetVisuallyDeemphasized(bool deemphasized);
  virtual bool ContainsNativeView(gfx::NativeView native_view) const;

  gfx::NativeView native_view() const { return view_.get(); }

  void Paint(const gfx::Rect&);

  // Called by GtkIMContextWrapper to forward a keyboard event to renderer.
  // Before calling RenderWidgetHost::ForwardKeyboardEvent(), this method
  // calls GtkKeyBindingsHandler::Match() against the event and send matched
  // edit commands to renderer by calling
  // RenderWidgetHost::ForwardEditCommandsForNextKeyEvent().
  void ForwardKeyboardEvent(const NativeWebKeyboardEvent& event);

  // Appends the input methods context menu to the specified |menu| object as a
  // submenu.
  void AppendInputMethodsContextMenu(MenuGtk* menu);

 private:
  friend class RenderWidgetHostViewGtkWidget;

  // Returns whether the widget needs an input grab (GTK+ and X) to work
  // properly.
  bool NeedsInputGrab();

  // Returns whether this render view is a popup (<select> dropdown or
  // autocomplete window).
  bool IsPopup();

  // Update the display cursor for the render view.
  void ShowCurrentCursor();

  // The model object.
  RenderWidgetHost* host_;

  // The native UI widget.
  OwnedWidgetGtk view_;

  // Cached value of --enable-gpu-rendering for out-of-process painting.
  bool enable_gpu_rendering_;

  // Non-NULL when we're doing out-of-process painting.
  scoped_ptr<GpuViewHost> gpu_view_host_;

  // This is true when we are currently painting and thus should handle extra
  // paint requests by expanding the invalid rect rather than actually
  // painting.
  bool about_to_validate_and_paint_;

  // This is the rectangle which we'll paint.
  gfx::Rect invalid_rect_;

  // Whether or not this widget is hidden.
  bool is_hidden_;

  // Whether we are currently loading.
  bool is_loading_;
  // The cursor for the page. This is passed up from the renderer.
  WebCursor current_cursor_;

  // Whether we are showing a context menu.
  bool is_showing_context_menu_;

  // The time at which this view started displaying white pixels as a result of
  // not having anything to paint (empty backing store from renderer). This
  // value returns true for is_null() if we are not recording whiteout times.
  base::TimeTicks whiteout_start_time_;

  // The time it took after this view was selected for it to be fully painted.
  base::TimeTicks tab_switch_paint_time_;

  // If true, fade the render widget when painting it.
  bool visually_deemphasized_;

  // Variables used only for popups --------------------------------------------
  // Our parent widget.
  RenderWidgetHostView* parent_host_view_;
  // The native view of our parent, equivalent to
  // parent_host_view_->GetNativeView().
  GtkWidget* parent_;
  // We ignore the first mouse release on popups.  This allows the popup to
  // stay open.
  bool is_popup_first_mouse_release_;

  // Whether or not this widget was focused before shadowed by another widget.
  // Used in OnGrabNotify() handler to track the focused state correctly.
  bool was_focused_before_grab_;

  // True if we are responsible for creating an X grab. This will only be used
  // for <select> dropdowns. It should be true for most such cases, but false
  // for extension popups.
  bool do_x_grab_;

  // A convenience wrapper object for GtkIMContext;
  scoped_ptr<GtkIMContextWrapper> im_context_;

  // A convenience object for handling editor key bindings defined in gtk
  // keyboard theme.
  scoped_ptr<GtkKeyBindingsHandler> key_bindings_handler_;

  // Helper class that lets us allocate plugin containers and move them.
  GtkPluginContainerManager plugin_container_manager_;

  // The size that we want the renderer to be.  We keep this in a separate
  // variable because resizing in GTK+ is async.
  gfx::Size requested_size_;

#if defined(OS_CHROMEOS)
  // Custimized tooltip window.
  scoped_ptr<views::TooltipWindowGtk> tooltip_window_;
#endif  // defined(OS_CHROMEOS)
};

#endif  // CHROME_BROWSER_RENDERER_HOST_RENDER_WIDGET_HOST_VIEW_GTK_H_