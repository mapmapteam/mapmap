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

ShapeGraphicsItem::ShapeGraphicsItem(Mapping::ptr mapping, bool output)
  : _mapping(mapping), _output(output)
{
  _shape = output ? getMapping()->getShape() : getMapping()->getInputShape();
}

MapperGLCanvas* ShapeGraphicsItem::getCanvas() const
{
  MainWindow* win = MainWindow::window();
  return isOutput() ? win->getDestinationCanvas() : win->getSourceCanvas();
}

bool ShapeGraphicsItem::isMappingCurrent() const {
  return MainWindow::window()->getCurrentMappingId() == getMapping()->getId();
}

bool ShapeGraphicsItem::isMappingVisible() const {
  return MainWindow::window()->getMappingManager().mappingIsVisible(getMapping());
}

void ShapeGraphicsItem::paint(QPainter *painter,
                              const QStyleOptionGraphicsItem *option, QWidget *widget)
{
  Q_UNUSED(widget);

  // Sync depth of figure with that of mapping (for layered output).
  if (isOutput())
    setZValue(MainWindow::window()->getMappingManager().getMappingDepth(getMapping()));
    //setZValue(getMapping()->getDepth());

  // Paint if visible.
  if (isMappingVisible())
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
//  if (!shapeParent->isMappingVisible())
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
//        MainWindow::instance()->setCurrentMapping(shapeParent->getMapping()->getId());
//      }
//    }
//    else
//    {
//      if (shapeParent->isMappingCurrent())
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
////    if (shapeParent->isMappingVisible() &&
////        shapeParent->isMappingCurrent())
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

  Color* color = static_cast<Color*>(getMapping()->getPaint().data());
  Q_ASSERT(color);

  painter->setPen(Qt::NoPen);

  // Set brush.
  QColor col = color->getColor();
  col.setAlphaF(getMapping()->getComputedOpacity());
  painter->setBrush(col);
}

PolygonColorGraphicsItem::PolygonColorGraphicsItem(Mapping::ptr mapping, bool output)
  : ColorGraphicsItem(mapping, output) {
  _controlPainter.reset(new PolygonControlPainter(this));
}

QPainterPath PolygonColorGraphicsItem::shape() const
{
  QPainterPath path;
  Polygon* poly = static_cast<Polygon*>(_shape.data());
  Q_ASSERT(poly);
  path.addPolygon(poly->toPolygon());
  return mapFromScene(path);
}

void PolygonColorGraphicsItem::_doPaint(QPainter *painter,
                                        const QStyleOptionGraphicsItem *option)
{
  Q_UNUSED(option);
  Polygon* poly = static_cast<Polygon*>(_shape.data());
  Q_ASSERT(poly);
  painter->drawPolygon(mapFromScene(poly->toPolygon()));
}

MeshColorGraphicsItem::MeshColorGraphicsItem(Mapping::ptr mapping, bool output)
: PolygonColorGraphicsItem(mapping, output)
{
  _controlPainter.reset(new MeshControlPainter(this));
}

void MeshColorGraphicsItem::_doPaint(QPainter *painter,
                                     const QStyleOptionGraphicsItem *option)
{
  Q_UNUSED(option);

  Mesh* mesh = static_cast<Mesh*>(_shape.data());
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

EllipseColorGraphicsItem::EllipseColorGraphicsItem(Mapping::ptr mapping, bool output)
  : ColorGraphicsItem(mapping, output) {
    _controlPainter.reset(new EllipseControlPainter(this));
  }

QPainterPath EllipseColorGraphicsItem::shape() const
{
  // Create path for ellipse.
  QPainterPath path;
  Ellipse* ellipse = static_cast<Ellipse*>(_shape.data());
  Q_ASSERT(ellipse);
  QTransform transform;
  transform.translate(ellipse->getCenter().x(), ellipse->getCenter().y());
  transform.rotate(ellipse->getRotation());
  path.addEllipse(QPoint(0,0), ellipse->getHorizontalRadius(), ellipse->getVerticalRadius());
  return mapFromScene(transform.map(path));
}

void EllipseColorGraphicsItem::_doPaint(QPainter* painter,
                                        const QStyleOptionGraphicsItem* option)
{
  Q_UNUSED(option);
  // Just draw the path.
  painter->drawPath(shape());
}

TextureGraphicsItem::TextureGraphicsItem(Mapping::ptr mapping, bool output)
  : ShapeGraphicsItem(mapping, output)
{
  _textureMapping = qSharedPointerCast<TextureMapping>(mapping);
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
  if (isMappingCurrent())
  {
    // FIXME: Does this draw the quad counterclockwise?
    glBegin (GL_QUADS);
    {
      QRectF rect = mapFromScene(_getTexture()->getRect()).boundingRect();

      Util::correctGlTexCoord(0, 0);
      glVertex3f (rect.x(), rect.y(), 0);

      Util::correctGlTexCoord(1, 0);
      glVertex3f (rect.x() + rect.width(), rect.y(), 0);

      Util::correctGlTexCoord(1, 1);
      glVertex3f (rect.x()+rect.width(), rect.y()+rect.height(), 0);

      Util::correctGlTexCoord(0, 1);
      glVertex3f (rect.x(), rect.y()+rect.height(), 0);
    }
    glEnd ();
  }
}

void TextureGraphicsItem::_prePaint(QPainter* painter,
                                    const QStyleOptionGraphicsItem *option)
{
	QSharedPointer<Texture> texture = _getTexture();
	Q_CHECK_PTR(texture);

  Q_UNUSED(option);
  painter->beginNativePainting();

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
  glBindTexture(GL_TEXTURE_2D, texture->getTextureId());

  // Copy bits to texture iff necessary.
  texture->lockMutex();
  if (texture->bitsHaveChanged())
  {
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA,
                 texture->getWidth(), texture->getHeight(), 0, GL_RGBA,
                 GL_UNSIGNED_BYTE, texture->getBits());
    // NOTE: We would gain in efficiency if we were able to just update the texture using glTexSubImage2D
    // See: http://stackoverflow.com/questions/11217121/how-to-manage-memory-with-texture-in-opengl
//    glTexSubImage2D(GL_TEXTURE_2D,
//        0, 0,
//        texture->getWidth(), texture->getHeight(), 0,
//        GL_RGBA, GL_UNSIGNED_BYTE, texture->getBits());
  }
  texture->unlockMutex();

  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

  // Set texture color (apply opacity).
  glColor4f(1.0f, 1.0f, 1.0f,
            isOutput() ? getMapping()->getComputedOpacity() : getMapping()->getPaint()->getOpacity());

}

void TextureGraphicsItem::_postPaint(QPainter* painter,
                                     const QStyleOptionGraphicsItem *option)
{
  Q_UNUSED(option);

  glDisable(GL_TEXTURE_2D);

  painter->endNativePainting();
}

QSharedPointer<Texture> TextureGraphicsItem::_getTexture()
{
  return qSharedPointerCast<Texture>(_textureMapping.toStrongRef()->getPaint());
}

QPainterPath PolygonTextureGraphicsItem::shape() const
{
  QPainterPath path;
  Polygon* poly = static_cast<Polygon*>(_shape.data());
  Q_ASSERT(poly);
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
    glBegin(GL_TRIANGLES);
    {
      for (int i=0; i<inputShape->nVertices(); i++)
      {
        Util::setGlTexPoint(*_getTexture(), inputShape->getVertex(i), mapFromScene(getShape()->getVertex(i)));
      }
    }
    glEnd();
  }
}

PolygonTextureGraphicsItem::PolygonTextureGraphicsItem(Mapping::ptr mapping, bool output) : TextureGraphicsItem(mapping, output) {
  _controlPainter.reset(new PolygonControlPainter(this));
}

MeshTextureGraphicsItem::MeshTextureGraphicsItem(Mapping::ptr mapping, bool output) : PolygonTextureGraphicsItem(mapping, output) {
  _controlPainter.reset(new MeshControlPainter(this));
  _nHorizontalQuads = _nVerticalQuads = -1;
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

    // Check if we increased or decreased number of columns/rows in mesh.
    bool forceRebuild = false;
    if (_nHorizontalQuads != outputMesh->nHorizontalQuads() ||
        _nVerticalQuads != outputMesh->nVerticalQuads())
    {
      forceRebuild = true;
      _cachedQuadItems.resize(_nHorizontalQuads = outputMesh->nHorizontalQuads());
      for (int i=0; i<_nHorizontalQuads; i++)
        _cachedQuadItems[i].resize(_nVerticalQuads = outputMesh->nVerticalQuads());
    }

    // Keep track of whether we are currently grabbing the shape or a vertex so as to
    // reduce resolution when editing (to prevent lags).
    bool grabbing = (isMappingCurrent() &&
                     (getCanvas()->shapeGrabbed() || getCanvas()->vertexGrabbed()));

    // Max depth is adjusted to draw less quads during click & drag.
    int maxDepth = (grabbing ? MM::MESH_SUBDIVISION_MAX_DEPTH_EDITING : MM::MESH_SUBDIVISION_MAX_DEPTH);

    // Force rebuild on shape/vertex release.
    if (_wasGrabbing && !grabbing) {
      forceRebuild = true;
    }
    _wasGrabbing = grabbing;

    // Go through the mesh quad by quad.
    for (int x = 0; x < outputMesh->nHorizontalQuads(); x++)
    {
      for (int y = 0; y < outputMesh->nVerticalQuads(); y++)
      {
        Quad::ptr inputQuad  = inputQuads[x][y];
        Quad::ptr outputQuad = outputQuads[x][y];

        // Verify if item needs recomputing.
        CacheQuadItem& item = _cachedQuadItems[x][y];
        if (forceRebuild ||
            item.parent.input->toPolygon()  != inputQuad->toPolygon() ||
            item.parent.output->toPolygon() != outputQuad->toPolygon()) {

          // Copy input and output quads for verification purposes.
          item.parent.input  = inputQuad;
          item.parent.output = outputQuad;

          // Recompute sub quads.
          item.subQuads.clear();

          QSizeF size = mapFromScene(outputQuad->toPolygon()).boundingRect().size();
          float area = size.width() * size.height();

          // Rebuild cache quad item.
          _buildCacheQuadItem(item, inputQuad, outputQuad, area, 0.0001f, 0.001f, MM::MESH_SUBDIVISION_MIN_AREA, maxDepth);
        }

        // Draw all the cached items.
        for (CacheQuadMapping m: item.subQuads)
        {
          glBegin(GL_QUADS);
          for (int i = 0; i < outputQuad->nVertices(); i++)
          {
            Util::setGlTexPoint(*_getTexture(), m.input->getVertex(i), mapFromScene(m.output->getVertex(i)));
          }
          glEnd();
        }
      }
    }
  }
}

void MeshTextureGraphicsItem::_buildCacheQuadItem(CacheQuadItem& item, const Quad::ptr& inputQuad, const Quad::ptr& outputQuad, float outputArea, float inputThreshod, float outputThreshold, int minArea, int maxDepth)
{
  bool stop = false;
  if (maxDepth == 0 || outputArea < minArea)
    stop = true;
  else {
    QPointF oa = mapFromScene(outputQuad->getVertex(0));
    QPointF ob = mapFromScene(outputQuad->getVertex(1));
    QPointF oc = mapFromScene(outputQuad->getVertex(2));
    QPointF od = mapFromScene(outputQuad->getVertex(3));

    QPointF ia = inputQuad->getVertex(0);
    QPointF ib = inputQuad->getVertex(1);
    QPointF ic = inputQuad->getVertex(2);
    QPointF id = inputQuad->getVertex(3);

    QPointF outputV1 = oa-ob;
    QPointF outputV2 = oc-ob;
    QPointF outputV3 = oc-od;
    QPointF outputV4 = oa-od;

    QPointF inputV1 = ia-ib;
    QPointF inputV2 = ic-ib;
    QPointF inputV3 = ic-id;
    QPointF inputV4 = ia-id;

    // compute the dot products for the polygon
    float outputV1dotV2 = QPointF::dotProduct(outputV1, outputV2);
    float outputV3dotV4 = QPointF::dotProduct(outputV3, outputV4);
    float outputV1dotV4 = QPointF::dotProduct(outputV1, outputV4);
    float outputV2dotV3 = QPointF::dotProduct(outputV2, outputV3);

    // compute the dot products for the texture
    float inputV1dotV2  = QPointF::dotProduct(inputV1, inputV2);
    float inputV3dotV4  = QPointF::dotProduct(inputV3, inputV4);
    float inputV1dotV4  = QPointF::dotProduct(inputV1, inputV4);
    float inputV2dotV3  = QPointF::dotProduct(inputV2, inputV3);

    // Stopping criterion.
    stop = (fabs(outputV1dotV2 - outputV3dotV4) < outputThreshold &&
            fabs(outputV1dotV4 - outputV2dotV3) < outputThreshold &&
            fabs(inputV1dotV2  - inputV3dotV4)  < inputThreshod &&
            fabs(inputV1dotV4  - inputV2dotV3)  < inputThreshod);
  }

  //
  if (stop)
  {
    item.subQuads.append( (CacheQuadMapping){ inputQuad, outputQuad } );
  }
  else // subdivide
  {
    QList<Quad::ptr> inputSubQuads  = _split(*inputQuad);
    QList<Quad::ptr> outputSubQuads = _split(*outputQuad);
    for (int i = 0; i < inputSubQuads.size(); i++)
    {
      _buildCacheQuadItem(item, inputSubQuads[i], outputSubQuads[i], outputArea*0.25, inputThreshod, outputThreshold, minArea, (maxDepth == -1 ? -1 : maxDepth - 1));
    }
  }
}

QList<Quad::ptr> MeshTextureGraphicsItem::_split(const Quad& quad)
{
  QList<Quad::ptr> quads;

  QPointF a = quad.getVertex(0);
  QPointF b = quad.getVertex(1);
  QPointF c = quad.getVertex(2);
  QPointF d = quad.getVertex(3);

  QPointF ab = (a + b) * 0.5f;
  QPointF bc = (b + c) * 0.5f;
  QPointF cd = (c + d) * 0.5f;
  QPointF ad = (a + d) * 0.5f;

  QPointF abcd = (ab + cd) * 0.5f;

  quads.append(Quad::ptr(new Quad(a, ab, abcd, ad)));
  quads.append(Quad::ptr(new Quad(ab, b, bc, abcd)));
  quads.append(Quad::ptr(new Quad(abcd, bc, c, cd)));
  quads.append(Quad::ptr(new Quad(ad, abcd, cd, d)));

  return quads;
}

EllipseTextureGraphicsItem::DrawingData::DrawingData(const QSharedPointer<Ellipse>& ellipse)
{
  // Gather basic definitions.
  center           = ellipse->getCenter();
  controlCenter    = ellipse->getVertex(4);
  horizontalRadius = ellipse->getHorizontalRadius();
  verticalRadius   = ellipse->getVerticalRadius();
  rotation         = ellipse->getRotationRadians();

  // Compute quarter angles.
  const QPointF& unitControlCenter  = ellipse->toUnitCircle().map(controlCenter);
  float inputAngle1 = asin(unitControlCenter.y());
  float inputAngle2 = acos(unitControlCenter.x());
  quarterAngles[0] = inputAngle1;
  quarterAngles[1] = inputAngle2;
  quarterAngles[2] = M_PI   - inputAngle1;
  quarterAngles[3] = 2*M_PI - inputAngle2;
}

float EllipseTextureGraphicsItem::DrawingData::getSpanInQuarter(int quarter) const
{
  float angleSpan = quarterAngles[(quarter+1)%N_QUARTERS] - quarterAngles[quarter];
  while (angleSpan < 0) angleSpan += 2*M_PI;
  return angleSpan;
}

void EllipseTextureGraphicsItem::DrawingData::setPointOfEllipseAtAngle(QPointF& point, float circularAngle)
{
  float xCirc = cos(circularAngle) * horizontalRadius; // this looks really weird...
  float yCirc = sin(circularAngle) * verticalRadius;
  float distance = sqrt( xCirc*xCirc + yCirc*yCirc );
  float angle    = atan2( yCirc, xCirc );
  point.setX( cos(angle + rotation) * distance + center.x() );
  point.setY( sin(angle + rotation) * distance + center.y() );
}

EllipseTextureGraphicsItem::EllipseTextureGraphicsItem(Mapping::ptr mapping, bool output) : TextureGraphicsItem(mapping, output) {
  _controlPainter.reset(new EllipseControlPainter(this));
}

QPainterPath EllipseTextureGraphicsItem::shape() const
{
  // Create path for ellipse.
  QPainterPath path;
  Ellipse* ellipse = static_cast<Ellipse*>(_shape.data());
  Q_ASSERT(ellipse);
  QTransform transform;
  transform.translate(ellipse->getCenter().x(), ellipse->getCenter().y());
  transform.rotate(ellipse->getRotation());
  path.addEllipse(QPoint(0,0), ellipse->getHorizontalRadius(), ellipse->getVerticalRadius());
  return mapFromScene(transform.map(path));
}

QRectF EllipseTextureGraphicsItem::boundingRect() const
{
  return shape().boundingRect();
}

void EllipseTextureGraphicsItem::_doDrawOutput(QPainter* painter)
{
  Q_UNUSED(painter);
  // Get input and output ellipses.
  QSharedPointer<Ellipse> inputEllipse  = qSharedPointerCast<Ellipse>(_inputShape);
  QSharedPointer<Ellipse> outputEllipse = qSharedPointerCast<Ellipse>(_shape);
  QSharedPointer<Texture> texture = _getTexture();

  // Data for calculating drawing.
  DrawingData inputData(inputEllipse);
  DrawingData outputData(outputEllipse);

  // Points that contain the triangle positions on the border of the ellipse.
  QPointF currentInputPoint;
  QPointF prevInputPoint(0, 0);
  QPointF currentOutputPoint;
  QPointF prevOutputPoint(0, 0);

  // Draw each quarter of the ellipse.
  for (int i=0; i<N_QUARTERS; i++)
  {
    // Total angle range of current quarter.
    float inputAngleSpanInQuarter  = inputData.getSpanInQuarter(i);
    float outputAngleSpanInQuarter = outputData.getSpanInQuarter(i);

    // N. triangles (computed according to output).
    int nTrianglesInQuarter  = ceil(outputAngleSpanInQuarter / (2*M_PI) * MM::ELLIPSE_N_TRIANGLES);

    // Angle per triangle.
    float inputAnglePerTriangle  = inputAngleSpanInQuarter / nTrianglesInQuarter;
    float outputAnglePerTriangle = outputAngleSpanInQuarter / nTrianglesInQuarter;

    float inputAngle  = inputData.quarterAngles[i];
    float outputAngle = outputData.quarterAngles[i];
    for (int j=0; j<=nTrianglesInQuarter; j++, inputAngle += inputAnglePerTriangle, outputAngle += outputAnglePerTriangle)
    {
      // Set next (current) points.
      inputData.setPointOfEllipseAtAngle(currentInputPoint, inputAngle);
      outputData.setPointOfEllipseAtAngle(currentOutputPoint, outputAngle);

      if (j > 0) // We don't draw the first triangle.
      {
        // Draw triangle.
        glBegin(GL_TRIANGLES);
        Util::setGlTexPoint(*texture, inputData.controlCenter, outputData.controlCenter);
        Util::setGlTexPoint(*texture, prevInputPoint,     prevOutputPoint);
        Util::setGlTexPoint(*texture, currentInputPoint,  currentOutputPoint);
        glEnd();
      }

      // Save point for next iteration.
      prevInputPoint.setX(currentInputPoint.x());
      prevInputPoint.setY(currentInputPoint.y());
      prevOutputPoint.setX(currentOutputPoint.x());
      prevOutputPoint.setY(currentOutputPoint.y());
    }

  }
}

}
