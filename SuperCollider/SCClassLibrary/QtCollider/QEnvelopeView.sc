QEnvelopeView : QView
{
  var <editable, <step, <grid, <gridOn = false;
  var <gridColor, <strokeColor, <fillColor, <selectionColor;
  var <drawLines = true, <drawRects = true;
  var <metaAction;

  *qtClass {^"QcGraph"}

  editable_ { arg aBool;
    editable = aBool;
    this.setProperty( \editable, aBool );
  }

  setEditable { arg index, flag;
    this.invokeMethod( \setEditableAt, [index, flag] );
  }

  step_ { arg aFloat;
    step = aFloat;
    this.setProperty( \step, aFloat );
  }

  value {
    ^this.getProperty( \value );
  }

  value_ { arg anArray;
    this.setProperty( \value, anArray );
  }

  valueAction_ { arg anArray;
    this.setProperty( \value, anArray );
    this.doAction;
  }

  index {
    ^this.getProperty( \index );
  }

  lastIndex {
    ^this.index;
  }

  selectIndex { arg anInt;
    this.setProperty( \index, anInt );
  }

  x {
    ^this.getProperty( \x );
  }

  x_ { arg aFloat;
    this.setProperty( \x, aFloat );
  }

  y {
    ^this.getProperty( \y );
  }

  y_ { arg aFloat;
    this.setProperty( \y, aFloat );
  }

  currentvalue { ^this.y }

  currentvalue_ { arg aFloat;
    this.y_( aFloat );
  }

  setString { arg index, string;
    this.invokeMethod( \setStringAt, [index, string] );
  }

  strings_ { arg anArray;
    this.setProperty( \strings, anArray );
  }

  curves { this.nonimpl( "curves" ); }

  curves_ { arg curves;
    this.invokeMethod( \setCurves,
                       if(curves.isKindOf( ArrayedCollection ) )
                        {[curves]}{[[curves]]} ); }

  setEnv { arg env;
    var times = [0] ++ env.times.integrate;
    if( times.last > 0 ) {times = times / times.last};
    this.value = [times, env.levels];
    this.curves = env.curves;
  }

  grid_ { arg aPoint;
    grid = aPoint;
    this.setProperty( \grid, aPoint );
  }

  gridOn_ { arg aBool;
    gridOn = aBool;
    this.setProperty( \gridOn, aBool );
  }

  gridColor_ { arg aColor;
    gridColor = aColor;
    this.setProperty( \gridColor, aColor );
  }

  connect { arg source, targets;
    this.invokeMethod( \connectElements, [source, targets] );
  }

  strokeColor_ { arg aColor;
    strokeColor = aColor;
    this.setProperty( \strokeColor, aColor );
  }

  fillColor_ { arg aColor;
    fillColor = aColor;
    this.setProperty( \fillColor, aColor );
  }

  setFillColor { arg index, color;
    this.invokeMethod( \setFillColorAt, [index, color] );
  }

  colors_ { arg strokeColor, fillColor;
    this.strokeColor_( strokeColor );
    this.fillColor_( fillColor );
  }

  selectionColor_ { arg aColor;
    selectionColor = aColor;
    this.setProperty( \selectionColor, aColor );
  }

  drawLines_ { arg aBool;
    drawLines = aBool;
    this.setProperty( \drawLines, aBool );
  }

  drawRects_ { arg aBool;
    drawRects = aBool;
    this.setProperty( \drawRects, aBool );
  }

  thumbWidth_ { arg aFloat;
    this.setProperty( \thumbWidth, aFloat; );
  }

  thumbHeight_ { arg aFloat;
    this.setProperty( \thumbHeight, aFloat; );
  }

  thumbSize_ { arg aFloat;
    this.setProperty( \thumbSize, aFloat; );
  }

  setThumbWidth { this.nonimpl("setThumbWidth"); }

  setThumbHeight { this.nonimpl("setThumbHeight"); }

  setThumbSize { this.nonimpl("setThumbSize"); }

  metaAction_ { arg function;
    this.manageMethodConnection( metaAction, function, 'metaAction()', \doMetaAction );
    metaAction = function;
  }

  doMetaAction {
    metaAction.value(this);
  }
}
