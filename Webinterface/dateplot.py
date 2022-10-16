import random
from datetime import datetime, timedelta
from bokeh.plotting import figure
from bokeh.embed import components
from bokeh.models import DatetimeTickFormatter, NumeralTickFormatter, Range1d
from bokeh.resources import INLINE
from bokeh.util.string import encode_utf8

def getStaticResources():
    js_resources = encode_utf8(INLINE.render_js())
    css_resources = encode_utf8(INLINE.render_css())
    return { "js_resources": js_resources, "css_resources" : css_resources }

def renderPlot(x, y, name):
    p = figure(
        title=name,
        x_axis_type="datetime",
        sizing_mode="scale_width",
        plot_height=9,
        plot_width=21
    )
    firstDate = x[0]
    lastDate = x[-1]
    p.x_range = Range1d(firstDate, lastDate, bounds='auto')
    p.y_range = Range1d(min(y) * 0.95, max(y) * 1.05, bounds='auto')
    p.toolbar.logo=None
    p.circle(x, y, size=5)
    p.line(x, y, color="blue", line_width=2)
    script, div = components(p)

    return encode_utf8(script), encode_utf8(div)