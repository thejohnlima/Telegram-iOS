#include "vpath.h"
#include <cassert>
#include <vector>
#include "vbezier.h"
#include "vdebug.h"
#include "vrect.h"

V_BEGIN_NAMESPACE

VPath::VPathData::VPathData()
    : m_points(), m_elements(), m_segments(0), mStartPoint(), mNewSegment(true)
{
}

VPath::VPathData::VPathData(const VPathData &o)
    : m_points(o.m_points),
      m_elements(o.m_elements),
      m_segments(o.m_segments),
      mStartPoint(o.mStartPoint),
      mNewSegment(o.mNewSegment)
{
}

void VPath::VPathData::transform(const VMatrix &m)
{
    for (auto &i : m_points) {
        i = m.map(i);
    }
}

float VPath::VPathData::length() const
{
    float len = 0.0;
    int   i = 0;
    for (auto e : m_elements) {
        switch (e) {
        case VPath::Element::MoveTo:
            i++;
            break;
        case VPath::Element::LineTo: {
            VPointF p0 = m_points[i - 1];
            VPointF p = m_points[i++];
            VBezier b = VBezier::fromPoints(p0, p0, p, p);
            len += b.length();
            break;
        }
        case VPath::Element::CubicTo: {
            VPointF p0 = m_points[i - 1];
            VPointF p = m_points[i++];
            VPointF p1 = m_points[i++];
            VPointF p2 = m_points[i++];
            VBezier b = VBezier::fromPoints(p0, p, p1, p2);
            len += b.length();
            break;
        }
        case VPath::Element::Close:
            break;
        }
    }

    return len;
}

void VPath::VPathData::checkNewSegment()
{
    if (mNewSegment) {
        moveTo(VPointF(0, 0));
        mNewSegment = false;
    }
}

void VPath::VPathData::moveTo(const VPointF &p)
{
    mStartPoint = p;
    mNewSegment = false;
    m_elements.push_back(VPath::Element::MoveTo);
    m_points.push_back(p);
    m_segments++;
}
void VPath::VPathData::lineTo(const VPointF &p)
{
    checkNewSegment();
    m_elements.push_back(VPath::Element::LineTo);
    m_points.push_back(p);
}
void VPath::VPathData::cubicTo(const VPointF &c1, const VPointF &c2,
                               const VPointF &e)
{
    checkNewSegment();
    m_elements.push_back(VPath::Element::CubicTo);
    m_points.push_back(c1);
    m_points.push_back(c2);
    m_points.push_back(e);
}

void VPath::VPathData::close()
{
    if (isEmpty()) return;

    const VPointF &lastPt = m_points.back();
    if (!fuzzyCompare(mStartPoint, lastPt)) {
        lineTo(mStartPoint);
    }
    m_elements.push_back(VPath::Element::Close);
    mNewSegment = true;
}

void VPath::VPathData::reset()
{
    if (isEmpty()) return;

    m_elements.clear();
    m_points.clear();
    m_segments = 0;
}

int VPath::VPathData::segments() const
{
    return m_segments + 1;
}

void VPath::VPathData::reserve(int pts, int elms)
{
    if (m_points.capacity() < m_points.size() + pts)
        m_points.reserve(m_points.size() + pts);
    if (m_elements.capacity() < m_elements.size() + elms)
        m_elements.reserve(m_elements.size() + elms);
}

static VPointF curvesForArc(const VRectF &, float, float, VPointF *, int *);
static constexpr float PATH_KAPPA = 0.5522847498;

void VPath::VPathData::arcTo(const VRectF &rect, float startAngle,
                             float sweepLength, bool forceMoveTo)
{
    int     point_count = 0;
    VPointF pts[15];
    VPointF curve_start =
        curvesForArc(rect, startAngle, sweepLength, pts, &point_count);

    reserve(point_count + 1, point_count / 3 + 1);
    if (isEmpty() || forceMoveTo) {
        moveTo(curve_start);
    } else {
        lineTo(curve_start);
    }
    for (int i = 0; i < point_count; i += 3) {
        cubicTo(pts[i], pts[i + 1], pts[i + 2]);
    }
}

void VPath::VPathData::addCircle(float cx, float cy, float radius,
                                 VPath::Direction dir)
{
    addOval(VRectF(cx - radius, cy - radius, 2 * radius, 2 * radius), dir);
}

void VPath::VPathData::addOval(const VRectF &rect, VPath::Direction dir)
{
    if (rect.isNull()) return;

    float x = rect.x();
    float y = rect.y();

    float w = rect.width();
    float w2 = rect.width() / 2;
    float w2k = w2 * PATH_KAPPA;

    float h = rect.height();
    float h2 = rect.height() / 2;
    float h2k = h2 * PATH_KAPPA;

    reserve(14, 7);  // 1Move + 4Cubic + 1Close
    if (dir == VPath::Direction::CW) {
        // moveto 12 o'clock.
        moveTo(VPointF(x + w2, y));
        // 12 -> 3 o'clock
        cubicTo(VPointF(x + w2 + w2k, y), VPointF(x + w, y + h2 - h2k),
                VPointF(x + w, y + h2));
        // 3 -> 6 o'clock
        cubicTo(VPointF(x + w, y + h2 + h2k), VPointF(x + w2 + w2k, y + h),
                VPointF(x + w2, y + h));
        // 6 -> 9 o'clock
        cubicTo(VPointF(x + w2 - w2k, y + h), VPointF(x, y + h2 + h2k),
                VPointF(x, y + h2));
        // 9 -> 12 o'clock
        cubicTo(VPointF(x, y + h2 - h2k), VPointF(x + w2 - w2k, y),
                VPointF(x + w2, y));
    } else {
        // moveto 12 o'clock.
        moveTo(VPointF(x + w2, y));
        // 12 -> 9 o'clock
        cubicTo(VPointF(x + w2 - w2k, y), VPointF(x, y + h2 - h2k),
                VPointF(x, y + h2));
        // 9 -> 6 o'clock
        cubicTo(VPointF(x, y + h2 + h2k), VPointF(x + w2 - w2k, y + h),
                VPointF(x + w2, y + h));
        // 6 -> 3 o'clock
        cubicTo(VPointF(x + w2 + w2k, y + h), VPointF(x + w, y + h2 + h2k),
                VPointF(x + w, y + h2));
        // 3 -> 12 o'clock
        cubicTo(VPointF(x + w, y + h2 - h2k), VPointF(x + w2 + w2k, y),
                VPointF(x + w2, y));
    }
    close();
}

void VPath::VPathData::addRect(const VRectF &rect, VPath::Direction dir)
{
    if (rect.isNull()) return;

    float x = rect.x();
    float y = rect.y();
    float w = rect.width();
    float h = rect.height();

    reserve(6, 6);  // 1Move + 4Line + 1Close
    if (dir == VPath::Direction::CW) {
        moveTo(VPointF(x + w, y));
        lineTo(VPointF(x + w, y + h));
        lineTo(VPointF(x, y + h));
        lineTo(VPointF(x, y));
        close();
    } else {
        moveTo(VPointF(x + w, y));
        lineTo(VPointF(x, y));
        lineTo(VPointF(x, y + h));
        lineTo(VPointF(x + w, y + h));
        close();
    }
}

void VPath::VPathData::addRoundRect(const VRectF &rect, float rx, float ry,
                                    VPath::Direction dir)
{
    if (vCompare(rx, 0.f) || vCompare(ry, 0.f)) {
        addRect(rect, dir);
        return;
    }

    float x = rect.x();
    float y = rect.y();
    float w = rect.width();
    float h = rect.height();
    // clamp the rx and ry radius value.
    rx = 2 * rx;
    ry = 2 * ry;
    if (rx > w) rx = w;
    if (ry > h) ry = h;

    reserve(14, 7);  // 1Move + 4Cubic + 1Close
    if (dir == VPath::Direction::CW) {
        moveTo(VPointF(x + w, y + ry / 2.f));
        arcTo(VRectF(x + w - rx, y + h - ry, rx, ry), 0, -90, false);
        arcTo(VRectF(x, y + h - ry, rx, ry), -90, -90, false);
        arcTo(VRectF(x, y, rx, ry), -180, -90, false);
        arcTo(VRectF(x + w - rx, y, rx, ry), -270, -90, false);
        close();
    } else {
        moveTo(VPointF(x + w, y + ry / 2.f));
        arcTo(VRectF(x + w - rx, y, rx, ry), 0, 90, false);
        arcTo(VRectF(x, y, rx, ry), 90, 90, false);
        arcTo(VRectF(x, y + h - ry, rx, ry), 180, 90, false);
        arcTo(VRectF(x + w - rx, y + h - ry, rx, ry), 270, 90, false);
        close();
    }
}

static float tForArcAngle(float angle);
void         findEllipseCoords(const VRectF &r, float angle, float length,
                               VPointF *startPoint, VPointF *endPoint)
{
    if (r.isNull()) {
        if (startPoint) *startPoint = VPointF();
        if (endPoint) *endPoint = VPointF();
        return;
    }

    float w2 = r.width() / 2;
    float h2 = r.height() / 2;

    float    angles[2] = {angle, angle + length};
    VPointF *points[2] = {startPoint, endPoint};

    for (int i = 0; i < 2; ++i) {
        if (!points[i]) continue;

        float theta = angles[i] - 360 * floor(angles[i] / 360);
        float t = theta / 90;
        // truncate
        int quadrant = int(t);
        t -= quadrant;

        t = tForArcAngle(90 * t);

        // swap x and y?
        if (quadrant & 1) t = 1 - t;

        float a, b, c, d;
        VBezier::coefficients(t, a, b, c, d);
        VPointF p(a + b + c * PATH_KAPPA, d + c + b * PATH_KAPPA);

        // left quadrants
        if (quadrant == 1 || quadrant == 2) p.rx() = -p.x();

        // top quadrants
        if (quadrant == 0 || quadrant == 1) p.ry() = -p.y();

        *points[i] = r.center() + VPointF(w2 * p.x(), h2 * p.y());
    }
}

static float tForArcAngle(float angle)
{
    float radians, cos_angle, sin_angle, tc, ts, t;

    if (vCompare(angle, 0.f)) return 0;
    if (vCompare(angle, 90.0f)) return 1;

    radians = (angle / 180) * M_PI;

    cos_angle = cos(radians);
    sin_angle = sin(radians);

    // initial guess
    tc = angle / 90;

    // do some iterations of newton's method to approximate cos_angle
    // finds the zero of the function b.pointAt(tc).x() - cos_angle
    tc -= ((((2 - 3 * PATH_KAPPA) * tc + 3 * (PATH_KAPPA - 1)) * tc) * tc + 1 -
           cos_angle)  // value
          / (((6 - 9 * PATH_KAPPA) * tc + 6 * (PATH_KAPPA - 1)) *
             tc);  // derivative
    tc -= ((((2 - 3 * PATH_KAPPA) * tc + 3 * (PATH_KAPPA - 1)) * tc) * tc + 1 -
           cos_angle)  // value
          / (((6 - 9 * PATH_KAPPA) * tc + 6 * (PATH_KAPPA - 1)) *
             tc);  // derivative

    // initial guess
    ts = tc;
    // do some iterations of newton's method to approximate sin_angle
    // finds the zero of the function b.pointAt(tc).y() - sin_angle
    ts -= ((((3 * PATH_KAPPA - 2) * ts - 6 * PATH_KAPPA + 3) * ts +
            3 * PATH_KAPPA) *
               ts -
           sin_angle) /
          (((9 * PATH_KAPPA - 6) * ts + 12 * PATH_KAPPA - 6) * ts +
           3 * PATH_KAPPA);
    ts -= ((((3 * PATH_KAPPA - 2) * ts - 6 * PATH_KAPPA + 3) * ts +
            3 * PATH_KAPPA) *
               ts -
           sin_angle) /
          (((9 * PATH_KAPPA - 6) * ts + 12 * PATH_KAPPA - 6) * ts +
           3 * PATH_KAPPA);

    // use the average of the t that best approximates cos_angle
    // and the t that best approximates sin_angle
    t = 0.5 * (tc + ts);
    return t;
}

// The return value is the starting point of the arc
static VPointF curvesForArc(const VRectF &rect, float startAngle,
                            float sweepLength, VPointF *curves,
                            int *point_count)
{
    if (rect.isNull()) {
        return VPointF();
    }

    float x = rect.x();
    float y = rect.y();

    float w = rect.width();
    float w2 = rect.width() / 2;
    float w2k = w2 * PATH_KAPPA;

    float h = rect.height();
    float h2 = rect.height() / 2;
    float h2k = h2 * PATH_KAPPA;

    VPointF points[16] = {
        // start point
        VPointF(x + w, y + h2),

        // 0 -> 270 degrees
        VPointF(x + w, y + h2 + h2k), VPointF(x + w2 + w2k, y + h),
        VPointF(x + w2, y + h),

        // 270 -> 180 degrees
        VPointF(x + w2 - w2k, y + h), VPointF(x, y + h2 + h2k),
        VPointF(x, y + h2),

        // 180 -> 90 degrees
        VPointF(x, y + h2 - h2k), VPointF(x + w2 - w2k, y), VPointF(x + w2, y),

        // 90 -> 0 degrees
        VPointF(x + w2 + w2k, y), VPointF(x + w, y + h2 - h2k),
        VPointF(x + w, y + h2)};

    if (sweepLength > 360)
        sweepLength = 360;
    else if (sweepLength < -360)
        sweepLength = -360;

    // Special case fast paths
    if (startAngle == 0.0) {
        if (sweepLength == 360.0) {
            for (int i = 11; i >= 0; --i) curves[(*point_count)++] = points[i];
            return points[12];
        } else if (sweepLength == -360.0) {
            for (int i = 1; i <= 12; ++i) curves[(*point_count)++] = points[i];
            return points[0];
        }
    }

    int startSegment = int(floor(startAngle / 90));
    int endSegment = int(floor((startAngle + sweepLength) / 90));

    float startT = (startAngle - startSegment * 90) / 90;
    float endT = (startAngle + sweepLength - endSegment * 90) / 90;

    int delta = sweepLength > 0 ? 1 : -1;
    if (delta < 0) {
        startT = 1 - startT;
        endT = 1 - endT;
    }

    // avoid empty start segment
    if (vIsZero(startT - float(1))) {
        startT = 0;
        startSegment += delta;
    }

    // avoid empty end segment
    if (vIsZero(endT)) {
        endT = 1;
        endSegment -= delta;
    }

    startT = tForArcAngle(startT * 90);
    endT = tForArcAngle(endT * 90);

    const bool splitAtStart = !vIsZero(startT);
    const bool splitAtEnd = !vIsZero(endT - float(1));

    const int end = endSegment + delta;

    // empty arc?
    if (startSegment == end) {
        const int quadrant = 3 - ((startSegment % 4) + 4) % 4;
        const int j = 3 * quadrant;
        return delta > 0 ? points[j + 3] : points[j];
    }

    VPointF startPoint, endPoint;
    findEllipseCoords(rect, startAngle, sweepLength, &startPoint, &endPoint);

    for (int i = startSegment; i != end; i += delta) {
        const int quadrant = 3 - ((i % 4) + 4) % 4;
        const int j = 3 * quadrant;

        VBezier b;
        if (delta > 0)
            b = VBezier::fromPoints(points[j + 3], points[j + 2], points[j + 1],
                                    points[j]);
        else
            b = VBezier::fromPoints(points[j], points[j + 1], points[j + 2],
                                    points[j + 3]);

        // empty arc?
        if (startSegment == endSegment && vCompare(startT, endT))
            return startPoint;

        if (i == startSegment) {
            if (i == endSegment && splitAtEnd)
                b = b.onInterval(startT, endT);
            else if (splitAtStart)
                b = b.onInterval(startT, 1);
        } else if (i == endSegment && splitAtEnd) {
            b = b.onInterval(0, endT);
        }

        // push control points
        curves[(*point_count)++] = b.pt2();
        curves[(*point_count)++] = b.pt3();
        curves[(*point_count)++] = b.pt4();
    }

    curves[*(point_count)-1] = endPoint;

    return startPoint;
}

void VPath::VPathData::addPolystar(float points, float innerRadius,
                                   float outerRadius, float innerRoundness,
                                   float outerRoundness, float startAngle,
                                   float cx, float cy, VPath::Direction dir)
{
    const static float POLYSTAR_MAGIC_NUMBER = 0.47829 / 0.28;
    float              currentAngle = (startAngle - 90.0) * M_PI / 180.0;
    float              x;
    float              y;
    float              previousX;
    float              previousY;
    float              partialPointRadius = 0;
    float              anglePerPoint = (float)(2.0 * M_PI / points);
    float              halfAnglePerPoint = anglePerPoint / 2.0;
    float              partialPointAmount = points - (int)points;
    bool               longSegment = false;
    int                numPoints = (int)ceil(points) * 2.0;
    float              angleDir = ((dir == VPath::Direction::CW) ? 1.0 : -1.0);
    bool               hasRoundness = false;

    innerRoundness /= 100.0;
    outerRoundness /= 100.0;

    if (partialPointAmount != 0) {
        currentAngle +=
            halfAnglePerPoint * (1.0 - partialPointAmount) * angleDir;
    }

    if (partialPointAmount != 0) {
        partialPointRadius =
            innerRadius + partialPointAmount * (outerRadius - innerRadius);
        x = (float)(partialPointRadius * cos(currentAngle));
        y = (float)(partialPointRadius * sin(currentAngle));
        currentAngle += anglePerPoint * partialPointAmount / 2.0 * angleDir;
    } else {
        x = (float)(outerRadius * cos(currentAngle));
        y = (float)(outerRadius * sin(currentAngle));
        currentAngle += halfAnglePerPoint * angleDir;
    }

    if (vIsZero(innerRoundness) && vIsZero(outerRoundness)) {
        reserve(numPoints + 2, numPoints + 3);
    } else {
        reserve(numPoints * 3 + 2, numPoints + 3);
        hasRoundness = true;
    }

    moveTo(VPointF(x + cx, y + cy));

    for (int i = 0; i < numPoints; i++) {
        float radius = longSegment ? outerRadius : innerRadius;
        float dTheta = halfAnglePerPoint;
        if (partialPointRadius != 0 && i == numPoints - 2) {
            dTheta = anglePerPoint * partialPointAmount / 2.0;
        }
        if (partialPointRadius != 0 && i == numPoints - 1) {
            radius = partialPointRadius;
        }
        previousX = x;
        previousY = y;
        x = (float)(radius * cos(currentAngle));
        y = (float)(radius * sin(currentAngle));

        if (hasRoundness) {
            float cp1Theta =
                (float)(atan2(previousY, previousX) - M_PI / 2.0 * angleDir);
            float cp1Dx = (float)cos(cp1Theta);
            float cp1Dy = (float)sin(cp1Theta);
            float cp2Theta = (float)(atan2(y, x) - M_PI / 2.0 * angleDir);
            float cp2Dx = (float)cos(cp2Theta);
            float cp2Dy = (float)sin(cp2Theta);

            float cp1Roundness = longSegment ? innerRoundness : outerRoundness;
            float cp2Roundness = longSegment ? outerRoundness : innerRoundness;
            float cp1Radius = longSegment ? innerRadius : outerRadius;
            float cp2Radius = longSegment ? outerRadius : innerRadius;

            float cp1x = cp1Radius * cp1Roundness * POLYSTAR_MAGIC_NUMBER *
                         cp1Dx / points;
            float cp1y = cp1Radius * cp1Roundness * POLYSTAR_MAGIC_NUMBER *
                         cp1Dy / points;
            float cp2x = cp2Radius * cp2Roundness * POLYSTAR_MAGIC_NUMBER *
                         cp2Dx / points;
            float cp2y = cp2Radius * cp2Roundness * POLYSTAR_MAGIC_NUMBER *
                         cp2Dy / points;

            if ((partialPointAmount != 0) &&
                ((i == 0) || (i == numPoints - 1))) {
                cp1x *= partialPointAmount;
                cp1y *= partialPointAmount;
                cp2x *= partialPointAmount;
                cp2y *= partialPointAmount;
            }

            cubicTo(VPointF(previousX - cp1x + cx, previousY - cp1y + cy),
                    VPointF(x + cp2x + cx, y + cp2y + cy),
                    VPointF(x + cx, y + cy));
        } else {
            lineTo(VPointF(x + cx, y + cy));
        }

        currentAngle += dTheta * angleDir;
        longSegment = !longSegment;
    }

    close();
}

void VPath::VPathData::addPolygon(float points, float radius, float roundness,
                                  float startAngle, float cx, float cy,
                                  VPath::Direction dir)
{
    // TODO: Need to support floating point number for number of points
    const static float POLYGON_MAGIC_NUMBER = 0.25;
    float              currentAngle = (startAngle - 90.0) * M_PI / 180.0;
    float              x;
    float              y;
    float              previousX;
    float              previousY;
    float              anglePerPoint = (float)(2.0 * M_PI / floor(points));
    int                numPoints = (int)floor(points);
    float              angleDir = ((dir == VPath::Direction::CW) ? 1.0 : -1.0);
    bool               hasRoundness = false;

    roundness /= 100.0;

    currentAngle = (currentAngle - 90.0) * M_PI / 180.0;
    x = (float)(radius * cos(currentAngle));
    y = (float)(radius * sin(currentAngle));
    currentAngle += anglePerPoint * angleDir;

    if (vIsZero(roundness)) {
        reserve(numPoints + 2, numPoints + 3);
    } else {
        reserve(numPoints * 3 + 2, numPoints + 3);
        hasRoundness = true;
    }

    moveTo(VPointF(x + cx, y + cy));

    for (int i = 0; i < numPoints; i++) {
        previousX = x;
        previousY = y;
        x = (float)(radius * cos(currentAngle));
        y = (float)(radius * sin(currentAngle));

        if (hasRoundness) {
            float cp1Theta =
                (float)(atan2(previousY, previousX) - M_PI / 2.0 * angleDir);
            float cp1Dx = (float)cos(cp1Theta);
            float cp1Dy = (float)sin(cp1Theta);
            float cp2Theta = (float)(atan2(y, x) - M_PI / 2.0 * angleDir);
            float cp2Dx = (float)cos(cp2Theta);
            float cp2Dy = (float)sin(cp2Theta);

            float cp1x = radius * roundness * POLYGON_MAGIC_NUMBER * cp1Dx;
            float cp1y = radius * roundness * POLYGON_MAGIC_NUMBER * cp1Dy;
            float cp2x = radius * roundness * POLYGON_MAGIC_NUMBER * cp2Dx;
            float cp2y = radius * roundness * POLYGON_MAGIC_NUMBER * cp2Dy;

            cubicTo(VPointF(previousX - cp1x + cx, previousY - cp1y + cy),
                    VPointF(x + cp2x + cx, y + cp2y + cy), VPointF(x, y));
        } else {
            lineTo(VPointF(x + cx, y + cy));
        }

        currentAngle += anglePerPoint * angleDir;
    }

    close();
}

V_END_NAMESPACE
