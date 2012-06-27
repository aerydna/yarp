// Generated by gtkmmproc -- DO NOT MODIFY!


#include <glibmm.h>

#include <gtkdataboxmm/ruler.h>
#include <gtkdataboxmm/private/ruler_p.h>


/* Copyright (c) 2012  Daniele E. Domenichelli <daniele.domenichelli@iit.it>
 *
 * This file is part of gtkdataboxmm.
 *
 * gtkdataboxmm is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published
 * by the Free Software Foundation, either version 2.1 of the License,
 * or (at your option) any later version.
 *
 * gtkdataboxmm is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <gtkdatabox_ruler.h>

namespace GDatabox
{

} // namespace GDatabox

namespace
{
} // anonymous namespace


namespace Glib
{

Glib::RefPtr<GDatabox::Ruler> wrap(GtkDataboxRuler* object, bool take_copy)
{
  return Glib::RefPtr<GDatabox::Ruler>( dynamic_cast<GDatabox::Ruler*> (Glib::wrap_auto ((GObject*)(object), take_copy)) );
  //We use dynamic_cast<> in case of multiple inheritance.
}

} /* namespace Glib */


namespace GDatabox
{


/* The *_Class implementation: */

const Glib::Class& Ruler_Class::init()
{
  if(!gtype_) // create the GType if necessary
  {
    // Glib::Class has to know the class init function to clone custom types.
    class_init_func_ = &Ruler_Class::class_init_function;

    // This is actually just optimized away, apparently with no harm.
    // Make sure that the parent type has been created.
    //CppClassParent::CppObjectType::get_type();

    // Create the wrapper type, with the same class/instance size as the base type.
    register_derived_type(gtk_databox_ruler_get_type());

    // Add derived versions of interfaces, if the C type implements any interfaces:

  }

  return *this;
}


void Ruler_Class::class_init_function(void* g_class, void* class_data)
{
  BaseClassType *const klass = static_cast<BaseClassType*>(g_class);
  CppClassParent::class_init_function(klass, class_data);


}


Glib::ObjectBase* Ruler_Class::wrap_new(GObject* object)
{
  return new Ruler((GtkDataboxRuler*)object);
}


/* The implementation: */

GtkDataboxRuler* Ruler::gobj_copy()
{
  reference();
  return gobj();
}

Ruler::Ruler(const Glib::ConstructParams& construct_params)
:
  Gtk::Widget(construct_params)
{

}

Ruler::Ruler(GtkDataboxRuler* castitem)
:
  Gtk::Widget((GtkWidget*)(castitem))
{}


Ruler::~Ruler()
{}


Ruler::CppClassType Ruler::ruler_class_; // initialize static member

GType Ruler::get_type()
{
  return ruler_class_.init().get_type();
}


GType Ruler::get_base_type()
{
  return gtk_databox_ruler_get_type();
}


Ruler::Ruler()
:
  // Mark this class as non-derived to allow C++ vfuncs to be skipped.
  Glib::ObjectBase(0),
  Gtk::Widget(Glib::ConstructParams(ruler_class_.init()))
{
  

}


} // namespace GDatabox


