/* -*- mode: C++ ; c-file-style: "stroustrup" -*- *****************************
 * Qwt Widget Library
 * Copyright (C) 1997   Josef Wilgen
 * Copyright (C) 2002   Uwe Rathmann
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the Qwt License, Version 1.0
 *****************************************************************************/

#include "qwt_plot_shapeitem.h"
#include "qwt_scale_map.h"
#include "qwt_painter.h"
#include "qwt_curve_fitter.h"

QPainterPath qwtTransformPath( const QwtScaleMap &xMap,
        const QwtScaleMap &yMap, const QPainterPath &path, bool doAlign )
{
    QPainterPath shape;
    shape.setFillRule( path.fillRule() );

    for ( int i = 0; i < path.elementCount(); i++ )
    {
        const QPainterPath::Element &element = path.elementAt( i );

        double x = xMap.transform( element.x );
        double y = yMap.transform( element.y );

        switch( element.type )
        {
            case QPainterPath::MoveToElement:
            {
                if ( doAlign )
                {
                    x = qRound( x );
                    y = qRound( y );
                }

                shape.moveTo( x, y );
                break;
            }
            case QPainterPath::LineToElement:
            {
                if ( doAlign )
                {
                    x = qRound( x );
                    y = qRound( y );
                }

                shape.lineTo( x, y );
                break;
            }
            case QPainterPath::CurveToElement:
            {
                const QPainterPath::Element& element1 = path.elementAt( ++i );
                const double x1 = xMap.transform( element1.x );
                const double y1 = yMap.transform( element1.y );

                const QPainterPath::Element& element2 = path.elementAt( ++i );
                const double x2 = xMap.transform( element2.x );
                const double y2 = yMap.transform( element2.y );

                shape.cubicTo( x, y, x1, y1, x2, y2 );
                break;
            }
            case QPainterPath::CurveToDataElement:
            {
                break;
            }
        }
    }

    return shape;
}


class QwtPlotShapeItem::PrivateData
{
public:
    PrivateData():
        renderTolerance( 0.0 )
    {
    }

    double renderTolerance;
    QRectF boundingRect;

    QPen pen;
    QBrush brush;
    QPainterPath shape;
};

/*!
   \brief Constructor

   Sets the following item attributes:
   - QwtPlotItem::AutoScale: true
   - QwtPlotItem::Legend:    false

   \param title Title
*/
QwtPlotShapeItem::QwtPlotShapeItem( const QString& title ):
    QwtPlotItem( QwtText( title ) )
{
    init();
}

/*!
   \brief Constructor

   Sets the following item attributes:
   - QwtPlotItem::AutoScale: true
   - QwtPlotItem::Legend:    false

   \param title Title
*/
QwtPlotShapeItem::QwtPlotShapeItem( const QwtText& title ):
    QwtPlotItem( title )
{
    init();
}

//! Destructor
QwtPlotShapeItem::~QwtPlotShapeItem()
{
    delete d_data;
}

void QwtPlotShapeItem::init()
{
    d_data = new PrivateData();
    d_data->boundingRect = QwtPlotItem::boundingRect();

    setItemAttribute( QwtPlotItem::AutoScale, true );
    setItemAttribute( QwtPlotItem::Legend, false );

    setZ( 8.0 );
}

//! \return QwtPlotItem::Rtti_PlotShape
int QwtPlotShapeItem::rtti() const
{
    return QwtPlotItem::Rtti_PlotShape;
}

//! Bounding rect of the item
QRectF QwtPlotShapeItem::boundingRect() const
{
    return d_data->boundingRect;
}

void QwtPlotShapeItem::setRect( const QRectF &rect ) 
{
    QPainterPath path;
    path.addRect( rect );

    setShape( path );
}

void QwtPlotShapeItem::setPolygon( const QPolygonF &polygon )
{
    QPainterPath shape;
    shape.addPolygon( polygon );

    setShape( shape );
}
    
void QwtPlotShapeItem::setShape( const QPainterPath &shape )
{
    if ( shape != d_data->shape )
    {
        d_data->shape = shape;
        if ( shape.isEmpty() )
        {
            d_data->boundingRect == QwtPlotItem::boundingRect();
        }
        else
        {
            d_data->boundingRect = shape.boundingRect();
        }

        itemChanged();
    }
}

QPainterPath QwtPlotShapeItem::shape() const
{
    return d_data->shape;
}

void QwtPlotShapeItem::setPen( const QPen &pen )
{
    if ( pen != d_data->pen )
    {
        d_data->pen = pen;
        itemChanged();
    }
}

QPen QwtPlotShapeItem::pen() const
{
    return d_data->pen;
}

void QwtPlotShapeItem::setBrush( const QBrush &brush )
{
    if ( brush != d_data->brush )
    {
        d_data->brush = brush;
        itemChanged();
    }
}

QBrush QwtPlotShapeItem::brush() const
{
    return d_data->brush;
}

void QwtPlotShapeItem::setRenderTolerance( double tolerance )
{
    tolerance = qMax( tolerance, 0.0 );

    if ( tolerance != d_data->renderTolerance )
    {
        d_data->renderTolerance = tolerance;
        itemChanged();
    }
}

double QwtPlotShapeItem::renderTolerance() const
{
    return d_data->renderTolerance;
}

QPainterPath QwtPlotShapeItem::simplifyPath( 
    const QPainterPath &path ) const
{
    if ( d_data->renderTolerance <= 0.0 )
        return path;

    QwtWeedingCurveFitter fitter( d_data->renderTolerance );

    QPainterPath shape;
    shape.setFillRule( path.fillRule() );

    const QList<QPolygonF> polygons = path.toSubpathPolygons();
    for ( int i = 0; i < polygons.size(); i++ )
        shape.addPolygon( fitter.fitCurve( polygons[ i ] ) );

    return shape;
}

/*!
  Draw the shape item

  \param painter Painter
  \param xMap X-Scale Map
  \param yMap Y-Scale Map
  \param canvasRect Contents rect of the plot canvas
*/
void QwtPlotShapeItem::draw( QPainter *painter,
    const QwtScaleMap &xMap, const QwtScaleMap &yMap,
    const QRectF &canvasRect ) const
{
    if ( d_data->shape.isEmpty() )
        return;

    if ( d_data->pen.style() == Qt::NoPen 
        && d_data->brush.style() == Qt::NoBrush )
    {
        return;
    }

    const QRectF cRect = QwtScaleMap::invTransform(
        xMap, yMap, canvasRect.toRect() );

    if ( d_data->boundingRect.intersects( cRect ) )
    {
        const bool doAlign = QwtPainter::roundingAlignment( painter );

        QPainterPath transformedShape = qwtTransformPath( xMap, yMap, 
            d_data->shape, doAlign );

        transformedShape = simplifyPath( transformedShape );

        painter->setPen( d_data->pen );
        painter->setBrush( d_data->brush );

        painter->drawPath( transformedShape );
    }
}