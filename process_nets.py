import json
from math import inf

def cast_to_layer(layer, dtype):
  if (layer == 67 and dtype == 44): return Layer(LI_CONTACT)
  if (layer == 68 and dtype == 20): return Layer(M1_ROUTE)
  if (layer == 68 and dtype == 44): return Layer(M1M2_VIA)
  if (layer == 69 and dtype == 20): return Layer(M2_ROUTE)
  if (layer == 69 and dtype == 44): return Layer(M2M3_VIA)
  if (layer == 70 and dtype == 20): return Layer(M3_ROUTE)
  if (layer == 70 and dtype == 44): return Layer(M3M4_VIA)
  if (layer == 71 and dtype == 20): return Layer(M4_ROUTE)
  if (layer == 71 and dtype == 44): return Layer(M4M5_VIA)
  if (layer == 72 and dtype == 20): return Layer(M5_ROUTE)


def get_winding(shape):
  pass

def point_inside_shape(xy, shape) :
  #shape is a list of xy coords [(x,y)]
  lspan = [inf,inf,inf,inf]
  rspan = [inf,inf,inf,inf]

  winding = get_winding(shape) # 1 for left interior else 0

  cur = shape[0]
  aft = shape[1]
  if(aft.y != cur.y):
     horizontal = 0
  else:
     horizontal = 1

  for edge in range(1,8):
    aft = shape[aft]
    if (horizontal == 1):
      if(xy[0] >= min(cur[0], aft[0]) and xy[0] <= max(cur[0], aft[0])):
        if(xy[1] == aft[0]):
          return True
        if (aft[0] < cur[0]) == (xy[1] < aft[1]):
          if(xy[1] > aft[1]):
            lspan[2] = min(lspan[2], xy[1] - aft[1])
          else:
            lspan[3] = min(lspan[3], aft[1] - xy[1])
        else:
          if(xy[1] > aft[1]):
            rspan[2] = min(rspan[2], xy[1] - aft[1])
          else:
            rspan[3] = min(rspan[3], aft[1] - xy[1])
      horizontal = 0
    else: #horizontal == 0
      if(xy[1] >= min(cur[1], aft[1]) and (xy[1] <= max(cur[1], aft[1]))):
        if(xy[0] == aft[0]):
           return True
        if (aft[1] > cur[1]) == (xy[1] < aft[1]):
          if (xy[0] > aft[0]):
            lspan[0] = min(lspan[0], xy[0] - aft[0])
          else:
            lspan[1] = min(lspan[1], aft[0] - xy[0])
        else:
          if(xy[0] > aft[0]):
            rspan[0] = min(rspan[0], xy[0] - aft[0])
          else:
            rspan[1] = min(rspan[1], aft[0] - xy[0])
      horizontal = 1
    cur = aft

    if (winding == 1):
      if((lspan[0] < rspan[0]) and (lspan[1] < rspan[1]) and (lspan[2] < rspan[2]) and (lspan[3] < rspan[3])):
        return True
    else:
      if((rspan[0] < lspan[0]) and (rspan[1] < lspan[1]) and (rspan[2] < lspan[2]) and (rspan[3] < lspan[3])):
        return True

    return False

class Layer(Enum):
    LI_CONTACT = 0
    M1_ROUTE = 1
    M1M2_VIA = 2
    M2_ROUTE = 3
    M2M3_VIA = 4
    M3_ROUTE = 5
    M3M4_VIA = 6
    M4_ROUTE = 7
    M4M5_VIA = 8
    M5_ROUTE = 9

class Net :
  self.map = {
    Rect : self._touch_rect,
    Pin  : self._touch_pin,
    Shape : self._touch_shape
  }

  def __init__ (self, layer, driver = None):
    self.name = ""
    self.layer = layer
    self.driver = driver # this net has a driver already
    self.segments = []
    self.loads = []      #contact inputs in the net

  def touches(self, net):
    return self.map[type(net)](net)

  def set_name(self, name):
    self.name = name

  def has_driver(self):
    return self.driver is not None

  def _touch_rect(self, net):
    pass
  def _touch_pin(self, net):
    pass
  def _touch_shape(self, net):
    pass

class Pin(Net):
  def __init__(self, xy, layer = LI_CONTACT, driver = None):
    super.__init__(self, layer = layer, driver = driver)
    self.xy = xy # tuple (x, y) pin center

  def _touch_rect(self, net):
    return (net.rect['x'][0] < self.xy.x and self.xy.x < net.rect['x'][1] and
            net.rect['y'][0] < self.xy.y and self.xy.y < net.rect['y'][1])
  def _touch_pin(self, net):
    #pin can't touch pin
    return False
  def _touch_shape(self, net):


class Rect(Net):
  def __init__(self, rect, layer = LI_CONTACT, driver = None):
    super.__init__(self, layer = layer, driver = driver)
    self.rect = rect # {'x': [x1, x2], 'y':[y1,y2]}

  def _touch_rect(self, net):

  def _touch_pin(self, net):

  def _touch_shape(self, net):



#grab the dict
in_file = open("./data/out.txt")
objects = json.load(in_file)

nets = []

outputs = [f'O[{i}]' for i in range(8)]
outputs.append('success')
inputs = ["I", "clk", "enable", "rst_n", "VGND", "VPWR"]

for io in objects['io'].keys()
  if io in inputs:
    new = Pin(objects['io'][io], layer = Layer(M3_ROUTE), driver = True)
    new.set_name(io)
    nets.append(new)
  else:
    new = Pin(objects['io'][io], layer = Layer(M3_ROUTE))

    nets.append(new)

for pin in objects['clist']:
  if (pin.driver == 1):
  nets.append(Pin(tuple(pin.loc), layer = Layer(LI_CONTACT), driver = pin.pinname))



