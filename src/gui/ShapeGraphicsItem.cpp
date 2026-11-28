/*
 * Shape.cpp
 *
 * (c) 2013 Sofian Audry -- info(@)sofianaudry(.)com
 * (c) 2013 Alexandre Quessy -- alexandre(@)quessy(.)net
 * (c) 2016 Dame Diongue -- baydamd(@)gmail(.)com
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

#include "ShapeGraphicsItem.h"

#include "MainWindow.h"

namespace mmp {

namespace {

/// Number of vertices of a triangle.
const int N_TRIANGLE_VERTICES = 3;

} // namespace

ShapeGraphicsItem::ShapeGraphicsItem(Layer::ptr mapping, bool output)
  : _mapping(mapping), _output(output)
{
  _shape = output ? getLayer()->getShape() : getLayer()->getInputShape();
}

MapperGLCanvas* ShapeGraphicsItem::getCanvas() const
{
  MainWindow* win = MainWindow::window();
  return isOutput() ? win->getDestinationCanvas() : win->getSourceCanvas();
}

bool ShapeGraphicsItem::isLayerCurrent() const {
  return MainWindow::window()->getCurrentLayerId() == getLayer()->getId();
}

bool ShapeGraphicsItem::isLayerVisible() const {
  return MainWindow::window()->getMappingManager().layerIsVisible(getLayer());
}

void ShapeGraphicsItem::paint(QPainter *painter,
                              const QStyleOptionGraphicsItem *option, QWidget *widget)
{
  Q_UNUSED(widget);

  // Sync depth of figure with that of mapping (for layered output).
  if (isOutput())
    setZValue(MainWindow::window()->getMappingManager().getLayerDepth(getLayer()));
    //setZValue(getLayer()->getDepth());

  // Paint if visible.
  if (isLayerVisible())
  {
    // Paint whatever needs to be painted.
    _prePaint(painter, option);
    _doPaint(painter, option);
    _postPaint(painter, option);
  }
}

//void VertexGraphicsItem::mousePressEvent(QGraphicsSceneMouseEvent * event)
//{
//  ShapeGraphicsItem* shapeParent = static_cast<ShapeGraphicsItem*>(parentItem());
//  if (!shapeParent->isLayerVisible())
//  {
//    // Prevent mouse grabbing.
//    event->ignore();
//  }
//  else
//  {
//    if (shapeParent->isOutput())
//    {
//      QGraphicsItem::mousePressEvent(event);
//      if (event->button() == Qt::LeftButton)
//      {
//        MainWindow::instance()->setCurrentMapping(shapeParent->getLayer()->getId());
//      }
//    }
//    else
//    {
//      if (shapeParent->isLayerCurrent())
//        QGraphicsItem::mousePressEvent(event);
//      else
//        event->ignore(); // prevent mousegrabbing on non-current mapping
//    }
//  }
//}
//
//void VertexGraphicsItem::paint(QPainter *painter,
//    const QStyleOptionGraphicsItem *option,
//    QWidget* widget)
//{
//  Q_UNUSED(widget);
////  if (MainWindow::instance()->displayControls())
////  {
////    ShapeGraphicsItem* shapeParent = static_cast<ShapeGraphicsItem*>(parentItem());
////    if (shapeParent->isLayerVisible() &&
////        shapeParent->isLayerCurrent())
////    {
////      qreal zoomFactor = 1.0 / shapeParent->getCanvas()->getZoomFactor();
////      resetMatrix();
////      scale(zoomFactor, zoomFactor);
////      Util::drawControlsVertex(painter, QPointF(0,0), (option->state & QStyle::State_Selected), MM::VERTEX_SELECT_RADIUS);
////    }
////  }
//}

void ColorGraphicsItem::_prePaint(QPainter *painter,
                                  const QStyleOptionGraphicsItem *option)
{
  Q_UNUSED(option);

  Color* color = static_cast<Color*>(getLayer()->getSource().data());
  Q_ASSERT(color);

  painter->setPen(Qt::NoPen);

  // Set brush.
  QColor col = color->getColor();
  col.setAlphaF(getLayer()->getComputedOpacity());
  painter->setBrush(col);
}

PolygonColorGraphicsItem::PolygonColorGraphicsItem(Layer::ptr mapping, bool output)
  : ColorGraphicsItem(mapping, output) {
  _controlPainter.reset(new PolygonControlPainter(this));
}

QPainterPath PolygonColorGraphicsItem::shape() const
{
  QPainterPath path;
  auto strong = _shape.toStrongRef();
  if (!strong) return path;
  Polygon* poly = static_cast<Polygon*>(strong.data());
  path.addPolygon(poly->toPolygon());
  return mapFromScene(path);
}

void PolygonColorGraphicsItem::_doPaint(QPainter *painter,
                                        const QStyleOptionGraphicsItem *option)
{
  Q_UNUSED(option);
  auto strong = _shape.toStrongRef();
  if (!strong) return;
  Polygon* poly = static_cast<Polygon*>(strong.data());
  painter->drawPolygon(mapFromScene(poly->toPolygon()));
}

MeshColorGraphicsItem::MeshColorGraphicsItem(Layer::ptr mapping, bool output)
: PolygonColorGraphicsItem(mapping, output)
{
  _controlPainter.reset(new MeshControlPainter(this));
}

void MeshColorGraphicsItem::_doPaint(QPainter *painter,
                                     const QStyleOptionGraphicsItem *option)
{
  Q_UNUSED(option);

  auto strong = _shape.toStrongRef();
  if (!strong) return;
  Mesh* mesh = static_cast<Mesh*>(strong.data());
  QVector<QVector<Quad::ptr> > quads = mesh->getQuads2d();

  // Go through the mesh quad by quad.
  for (int x = 0; x < mesh->nHorizontalQuads(); x++)
  {
    for (int y = 0; y < mesh->nVerticalQuads(); y++)
    {
      Quad::ptr quad = quads[x][y];

      painter->drawPolygon(mapFromScene(quad->toPolygon()));
    }
  }
}

EllipseColorGraphicsItem::EllipseColorGraphicsItem(Layer::ptr mapping, bool output)
  : ColorGraphicsItem(mapping, output) {
    _controlPainter.reset(new EllipseControlPainter(this));
  }

QPainterPath EllipseColorGraphicsItem::shape() const
{
  // Create path for ellipse.
  QPainterPath path;
  auto strong = _shape.toStrongRef();
  if (!strong) return path;
  Ellipse* ellipse = static_cast<Ellipse*>(strong.data());
  path.addPolygon(ellipse->toPolygon());
  path.closeSubpath();
  return mapFromScene(path);
}

void EllipseColorGraphicsItem::_doPaint(QPainter* painter,
                                        const QStyleOptionGraphicsItem* option)
{
  Q_UNUSED(option);
  // Just draw the path.
  painter->drawPath(shape());
}

TextureGraphicsItem::TextureGraphicsItem(Layer::ptr mapping, bool output)
  : ShapeGraphicsItem(mapping, output)
{
  _textureMapping = qSharedPointerCast<TextureLayer>(mapping);
  Q_CHECK_PTR(_textureMapping);

  _inputShape = qSharedPointerCast<MShape>(_textureMapping.toStrongRef()->getInputShape());
  Q_CHECK_PTR(_inputShape);
}

void TextureGraphicsItem::_doPaint(QPainter *painter,
                                   const QStyleOptionGraphicsItem *option)
{
  Q_UNUSED(option);
  // Perform the actual mapping (done by subclasses).
  if (isOutput())
    _doDrawOutput(painter);
  else
    _doDrawInput(painter);
}

void TextureGraphicsItem::_doDrawInput(QPainter* painter)
{
  Q_UNUSED(painter);
  if (isLayerCurrent())
  {
    const QRectF rect = mapFromScene(_getTexture()->getRect()).boundingRect();
    _drawTriangles(ShapeGeometry::rectangle(rect));
  }
}

void TextureGraphicsItem::_prePaint(QPainter* painter,
                                    const QStyleOptionGraphicsItem *option)
{
	QSharedPointer<Texture> texture = _getTexture();
	Q_CHECK_PTR(texture);

  Q_UNUSED(option);
  painter->beginNativePainting();

  // Free any textures orphaned by destroyed sources now that a GL context is
  // current (see issue #229).
  Texture::deleteOrphanedTextures();

  // Project source texture and sent it to destination.
  texture->update();

  // Only works for similar shapes.
  // TODO:remettre
  //Q_ASSERT( _inputShape->nVertices() == outputShape->nVertices());

  // Allow alpha blending.
  glEnable (GL_BLEND);
  glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

  // Get texture.
  glEnable (GL_TEXTURE_2D);
  // Uploads new content if needed (YUV video is converted by a shader).
  texture->bind();

  // Set texture color (apply opacity).
  _opacity = isOutput() ? getLayer()->getComputedOpacity() : getLayer()->getSource()->getOpacity();
  glColor4f(1.0f, 1.0f, 1.0f, _opacity);

  _viewMatrix = ShapeRenderer::viewMatrix(painter->deviceTransform(), ShapeRenderer::currentViewportSize());
}

void TextureGraphicsItem::_drawTriangles(const QVector<TexturedVertex>& vertices)
{
  QSharedPointer<Texture> texture = _getTexture();
  if (texture)
    ShapeRenderer::drawTriangles(vertices, _viewMatrix, _opacity, texture->boundYuv());
}

void TextureGraphicsItem::_postPaint(QPainter* painter,
                                     const QStyleOptionGraphicsItem *option)
{
  Q_UNUSED(option);

  _getTexture()->release();
  glDisable(GL_TEXTURE_2D);

  painter->endNativePainting();
}

QSharedPointer<Texture> TextureGraphicsItem::_getTexture()
{
  auto strong = _textureMapping.toStrongRef();
  if (!strong) return QSharedPointer<Texture>();
  return qSharedPointerCast<Texture>(strong->getSource());
}

QPainterPath PolygonTextureGraphicsItem::shape() const
{
  QPainterPath path;
  auto strong = _shape.toStrongRef();
  if (!strong) return path;
  Polygon* poly = static_cast<Polygon*>(strong.data());
  path.addPolygon(poly->toPolygon());
  return mapFromScene(path);
}

QRectF PolygonTextureGraphicsItem::boundingRect() const {
  return shape().boundingRect();
}

void TriangleTextureGraphicsItem::_doDrawOutput(QPainter* painter)
{
  Q_UNUSED(painter);
  if (isOutput())
  {
    MShape::ptr inputShape = _inputShape.toStrongRef();
    MShape::ptr outputShape = getShape();
    if (!inputShape || !outputShape ||
        inputShape->nVertices() < N_TRIANGLE_VERTICES || outputShape->nVertices() < N_TRIANGLE_VERTICES)
      return;
    QPointF input[N_TRIANGLE_VERTICES];
    QPointF output[N_TRIANGLE_VERTICES];
    for (int i = 0; i < N_TRIANGLE_VERTICES; i++)
    {
      input[i]  = inputShape->getVertex(i);
      output[i] = mapFromScene(outputShape->getVertex(i));
    }
    _drawTriangles(ShapeGeometry::triangle(_getTexture()->getRect(), input, output));
  }
}

PolygonTextureGraphicsItem::PolygonTextureGraphicsItem(Layer::ptr mapping, bool output) : TextureGraphicsItem(mapping, output) {
  _controlPainter.reset(new PolygonControlPainter(this));
}

MeshTextureGraphicsItem::MeshTextureGraphicsItem(Layer::ptr mapping, bool output) : PolygonTextureGraphicsItem(mapping, output) {
  _controlPainter.reset(new MeshControlPainter(this));
  _nHorizontalQuads = _nVerticalQuads = -1;
  _gridsDiffered = false;
  _wasGrabbing = false;
}

void MeshTextureGraphicsItem::_doDrawOutput(QPainter* painter)
{
  Q_UNUSED(painter);
  if (isOutput())
  {
    QSharedPointer<Mesh> outputMesh = qSharedPointerCast<Mesh>(_shape);
    QSharedPointer<Mesh> inputMesh  = qSharedPointerCast<Mesh>(_inputShape);
    QVector<QVector<Quad::ptr> > outputQuads = outputMesh->getQuads2d();
    QVector<QVector<Quad::ptr> > inputQuads  = inputMesh->getQuads2d();

    // A project written by hand may give the two meshes different grids (editing one grid
    // resizes the other, see Layer): only the quads they share are drawn.
    const int nHorizontalQuads = qMin(outputMesh->nHorizontalQuads(), inputMesh->nHorizontalQuads());
    const int nVerticalQuads   = qMin(outputMesh->nVerticalQuads(),   inputMesh->nVerticalQuads());
    const bool gridsDiffer = outputMesh->nHorizontalQuads() != inputMesh->nHorizontalQuads() ||
                             outputMesh->nVerticalQuads()   != inputMesh->nVerticalQuads();
    if (gridsDiffer && !_gridsDiffered)
      qWarning() << "Layer" << getLayer()->getId() << "has an output mesh and an input mesh on different grids:"
                 << "the quads of the larger one that the other lacks are not drawn.";
    _gridsDiffered = gridsDiffer;

    // Check if we increased or decreased number of columns/rows in mesh.
    bool forceRebuild = false;
    if (_nHorizontalQuads != nHorizontalQuads ||
        _nVerticalQuads != nVerticalQuads)
    {
      forceRebuild = true;
      _nHorizontalQuads = nHorizontalQuads;
      _nVerticalQuads = nVerticalQuads;
      _cachedQuadItems.resize(_nHorizontalQuads);
      for (int i=0; i<_nHorizontalQuads; i++)
        _cachedQuadItems[i].resize(_nVerticalQuads);
    }

    // Keep track of whether we are currently grabbing the shape or a vertex so as to
    // reduce resolution when editing (to prevent lags).
    bool grabbing = (isLayerCurrent() &&
                     (getCanvas()->shapeGrabbed() || getCanvas()->vertexGrabbed()));

    // Max depth is adjusted to draw less quads during click & drag.
    int maxDepth = (grabbing ? MM::MESH_SUBDIVISION_MAX_DEPTH_EDITING : MM::MESH_SUBDIVISION_MAX_DEPTH);

    // Force rebuild on shape/vertex release.
    if (_wasGrabbing && !grabbing) {
      forceRebuild = true;
    }
    _wasGrabbing = grabbing;

    const QRectF textureRect = _getTexture()->getRect();
    QVector<TexturedVertex> vertices;

    // Go through the mesh quad by quad.
    for (int x = 0; x < nHorizontalQuads; x++)
    {
      for (int y = 0; y < nVerticalQuads; y++)
      {
        Quad::ptr inputQuad  = inputQuads[x][y];
        Quad::ptr outputQuad = outputQuads[x][y];
        const QPolygonF inputPolygon  = inputQuad->toPolygon();
        const QPolygonF outputPolygon = outputQuad->toPolygon();

        // Verify if item needs recomputing.
        CacheQuadItem& item = _cachedQuadItems[x][y];
        if (forceRebuild ||
            item.input  != inputPolygon ||
            item.output != outputPolygon) {

          // Copy input and output quads for verification purposes.
          item.input  = inputPolygon;
          item.output = outputPolygon;

          // Recompute sub quads.
          item.subQuads.clear();
          if (inputPolygon.size() == QuadCorners::N_CORNERS && outputPolygon.size() == QuadCorners::N_CORNERS)
          {
            const QPolygonF drawnPolygon = mapFromScene(outputPolygon);
            QuadCorners corners;
            for (int i = 0; i < QuadCorners::N_CORNERS; i++)
            {
              corners.input[i]  = inputPolygon[i];
              corners.output[i] = drawnPolygon[i];
            }
            const QSizeF size = drawnPolygon.boundingRect().size();
            ShapeGeometry::subdivideQuad(corners, size.width() * size.height(),
                                         MM::MESH_SUBDIVISION_MIN_AREA, maxDepth, item.subQuads);
          }
        }

        for (const QuadCorners& subQuad : item.subQuads)
          ShapeGeometry::appendQuad(textureRect, subQuad, vertices);
      }
    }

    Util::drawTexturedTriangles(vertices);
  }
}

EllipseTextureGraphicsItem::EllipseTextureGraphicsItem(Layer::ptr mapping, bool output) : TextureGraphicsItem(mapping, output) {
  _controlPainter.reset(new EllipseControlPainter(this));
}

QPainterPath EllipseTextureGraphicsItem::shape() const
{
  // Create path for ellipse.
  QPainterPath path;
  auto strong = _shape.toStrongRef();
  if (!strong) return path;
  Ellipse* ellipse = static_cast<Ellipse*>(strong.data());
  path.addPolygon(ellipse->toPolygon());
  path.closeSubpath();
  return mapFromScene(path);
}

QRectF EllipseTextureGraphicsItem::boundingRect() const
{
  return shape().boundingRect();
}

void EllipseTextureGraphicsItem::_doDrawOutput(QPainter* painter)
{
  Q_UNUSED(painter);
  // Get input and output ellipses.
  QSharedPointer<Ellipse> inputEllipse  = qSharedPointerCast<Ellipse>(_inputShape.toStrongRef());
  QSharedPointer<Ellipse> outputEllipse = qSharedPointerCast<Ellipse>(_shape.toStrongRef());
  QSharedPointer<Texture> texture = _getTexture();
  if (!inputEllipse || !outputEllipse || !texture)
    return;

  // Projective mapping output -> unit circle -> input.
  const QTransform outputToInput = outputEllipse->toUnitCircle() * inputEllipse->fromUnitCircle();
  const QTransform fromUnitCircle = outputEllipse->fromUnitCircle();

  // Draw a fan of triangles around the perspective center.
  Util::drawTexturedTriangles(ShapeGeometry::ellipse(texture->getRect(), outputToInput, fromUnitCircle,
                                                     outputEllipse->getCenter(), sceneTransform().inverted(),
                                                     MM::ELLIPSE_N_TRIANGLES));
}

}
