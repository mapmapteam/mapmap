/*
 * OscAction.cpp
 *
 * (c) 2026 Alexandre Quessy -- alexandre(@)quessy(.)net
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "OscAction.h"

#include <QStringList>

namespace mmp {

namespace {

OscAction invalid(const QString& reason)
{
  OscAction a;
  a.type = OscAction::Invalid;
  a.error = reason;
  return a;
}

// Fills in the target selector of `a` from the first OSC argument.
// A string argument selects element(s) by (wildcard) name; any other type
// selects a single element by integer id.
void parseSelector(OscAction& a, const QVariant& selector)
{
  if (selector.typeId() == QMetaType::QString)
  {
    a.selectByName = true;
    a.name = selector.toString();
  }
  else
  {
    a.selectByName = false;
    a.id = selector.toInt();
  }
}

OscAction parseSource(const QStringList& sub, const QVariantList& args)
{
  // Every per-source command needs at least the target selector.
  if (args.isEmpty())
    return invalid("source command is missing its target (id or name)");

  if (sub.isEmpty())
    return invalid("source command is missing a subcommand");

  OscAction a;
  parseSelector(a, args.at(0));

  if (sub.size() == 1 && sub.at(0) == QLatin1String("play"))
  {
    a.type = OscAction::SourcePlay;
    return a;
  }
  if (sub.size() == 1 && sub.at(0) == QLatin1String("pause"))
  {
    a.type = OscAction::SourcePause;
    return a;
  }
  if (sub.size() == 1 && sub.at(0) == QLatin1String("rewind"))
  {
    a.type = OscAction::SourceRewind;
    return a;
  }

  // Otherwise a single token is treated as a property name to set.
  if (sub.size() == 1)
  {
    if (args.size() < 2)
      return invalid("source property '" + sub.at(0) + "' is missing its value");
    a.type = OscAction::SourceProperty;
    a.property = sub.at(0);
    a.value = args.at(1);
    return a;
  }

  return invalid("unknown source subcommand: " + sub.join('/'));
}

OscAction parseLayer(const QStringList& sub, const QVariantList& args)
{
  if (args.isEmpty())
    return invalid("layer command is missing its target (id or name)");

  if (sub.isEmpty())
    return invalid("layer command is missing a subcommand");

  OscAction a;
  parseSelector(a, args.at(0));

  // Absolute move: /mapmap/layer/move/xy <sel> <x> <y>
  if (sub.size() == 2 && sub.at(0) == QLatin1String("move") && sub.at(1) == QLatin1String("xy"))
  {
    if (args.size() < 3)
      return invalid("layer move/xy needs <selector> <x> <y>");
    a.type = OscAction::LayerMove;
    a.x = args.at(1).toDouble();
    a.y = args.at(2).toDouble();
    return a;
  }

  // Relative translate: /mapmap/layer/translate/xy <sel> <dx> <dy>
  if (sub.size() == 2 && sub.at(0) == QLatin1String("translate") && sub.at(1) == QLatin1String("xy"))
  {
    if (args.size() < 3)
      return invalid("layer translate/xy needs <selector> <dx> <dy>");
    a.type = OscAction::LayerTranslate;
    a.x = args.at(1).toDouble();
    a.y = args.at(2).toDouble();
    return a;
  }

  // Vertex edit: /mapmap/layer/vertex[/source|/destination]/xy <sel> <index> <x> <y>
  if (sub.first() == QLatin1String("vertex") && sub.last() == QLatin1String("xy"))
  {
    a.shapeRole = OscAction::OutputShape;
    bool roleOk = true;
    if (sub.size() == 2) // vertex/xy
      a.shapeRole = OscAction::OutputShape;
    else if (sub.size() == 3 && sub.at(1) == QLatin1String("destination"))
      a.shapeRole = OscAction::OutputShape;
    else if (sub.size() == 3 && sub.at(1) == QLatin1String("source"))
      a.shapeRole = OscAction::InputShape;
    else
      roleOk = false;

    if (!roleOk)
      return invalid("unknown layer vertex subcommand: " + sub.join('/'));

    if (args.size() < 4)
      return invalid("layer vertex/xy needs <selector> <index> <x> <y>");
    a.type = OscAction::LayerVertex;
    a.vertexIndex = args.at(1).toInt();
    a.x = args.at(2).toDouble();
    a.y = args.at(3).toDouble();
    return a;
  }

  // Otherwise a single token is treated as a property name to set.
  if (sub.size() == 1)
  {
    if (args.size() < 2)
      return invalid("layer property '" + sub.at(0) + "' is missing its value");
    a.type = OscAction::LayerProperty;
    a.property = sub.at(0);
    a.value = args.at(1);
    return a;
  }

  return invalid("unknown layer subcommand: " + sub.join('/'));
}

} // namespace

OscAction parseOscAction(const QString& address, const QVariantList& args)
{
  // Tokenize the address, dropping empty tokens (leading '/', double '//').
  const QStringList tokens = address.split('/', Qt::SkipEmptyParts);

  if (tokens.isEmpty() || tokens.first() != QLatin1String("mapmap"))
    return invalid("address does not start with /mapmap");

  const QStringList rest = tokens.mid(1);
  if (rest.isEmpty())
    return invalid("address has no command after /mapmap");

  const QString& head = rest.first();

  // Global transport (no target).
  if (rest.size() == 1)
  {
    if (head == QLatin1String("play"))   { OscAction a; a.type = OscAction::PlayAll;   return a; }
    if (head == QLatin1String("pause"))  { OscAction a; a.type = OscAction::PauseAll;  return a; }
    if (head == QLatin1String("rewind")) { OscAction a; a.type = OscAction::RewindAll; return a; }
    if (head == QLatin1String("quit"))   { OscAction a; a.type = OscAction::Quit;      return a; }
  }

  if (head == QLatin1String("source"))
    return parseSource(rest.mid(1), args);

  // "mapping" is accepted as an alias of "layer".
  if (head == QLatin1String("layer") || head == QLatin1String("mapping"))
    return parseLayer(rest.mid(1), args);

  return invalid("unknown command: " + address);
}

} // namespace mmp
