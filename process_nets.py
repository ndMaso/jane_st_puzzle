import json
from math import inf
from functools import cmp_to_key
from enum import IntEnum

def sign(num):
  return num != abs(num)

def cast_to_layer(layer, dtype):
  if (layer == 67 and dtype == 44): return Layer.LI_CONTACT
  if (layer == 68 and dtype == 20): return Layer.M1_ROUTE
  if (layer == 68 and dtype == 44): return Layer.M1M2_VIA
  if (layer == 69 and dtype == 20): return Layer.M2_ROUTE
  if (layer == 69 and dtype == 44): return Layer.M2M3_VIA
  if (layer == 70 and dtype == 20): return Layer.M3_ROUTE
  if (layer == 70 and dtype == 44): return Layer.M3M4_VIA
  if (layer == 71 and dtype == 20): return Layer.M4_ROUTE
  if (layer == 71 and dtype == 44): return Layer.M4M5_VIA
  if (layer == 72 and dtype == 20): return Layer.M5_ROUTE


def get_winding(shape):
  cur = shape[0]
  x1min = cur[0]
  x2max = cur[0]
  y1min = cur[1]
  y2max = cur[1]

  for p in range(1,len(shape)):
    this = shape[p]
    x1min = x1min if (x1min < this[0]) else this[0]
    x2max = x2max if (x2max > this[0]) else this[0]
    y1min = y1min if (y1min < this[1]) else this[1]
    y2max = y2max if (y2max > this[1]) else this[1]

    if (cur[0] == this[0]):
      if (this[0] == x1min):
        down_upb = 0 if (this[1] > cur[1]) else 1

    cur = this

  return down_upb

def point_inside_shape(shape, xy) :
  #shape is a list of xy coords [(x,y)]
  lspan = [inf,inf,inf,inf]
  rspan = [inf,inf,inf,inf]

  winding = get_winding(shape) # 1 for left interior else 0

  cur = shape[0]
  if(shape[0][1] != shape[1][1]):
     horizontal = 0
  else:
     horizontal = 1

  for edge in range(1,len(shape)):
    aft = shape[edge]
    if (horizontal == 1):
      if ((xy[0] >= min(cur[0], aft[0])) and (xy[0] <= max(cur[0], aft[0]))):
        if(xy[1] == aft[1]):
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
      #print(f"hi {xy}")
      if ((xy[1] >= min(cur[1], aft[1])) and ((xy[1] <= max(cur[1], aft[1])))):
        if(xy[0] == aft[0]):
           return True
        if (aft[1] > cur[1]) == (xy[0] < aft[0]):
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

def shape_touch_rect(shape, rect):
  #shape may intersect with rect if one entirely covers the other, or if any of the
    #vertical edges of the rect touch any of the horizontal edges of the shape OR VICE VERSA

    #check a corner of the rect is inside the shape
    if (point_inside_shape(shape, [rect['x'][0], rect['y'][0]])):
      return True
    #check a point of the shape inside the rect
    if (shape[0][0] >= rect['x'][0] and shape[0][0] <= rect['x'][1] and
        shape[0][1] >= rect['y'][0] and shape[0][1] <= rect['y'][1]):
      return True

    #check all horizontal edges of the shape intersect the vertical edges of the rect
    if(shape[0][1] == shape[1][1]): #first edge is horizontal
      horiz = 1
    else:
      horiz = 0
    cur = 0
    while(cur < (len(shape)-1)):    #   |x1            |x2
      aft = cur + 1                 #   |              |
                                    #   |         c____+_____a
                                    #   |              |
      if horiz==1:
        if (((sign(shape[cur][0] - rect['x'][0]) != sign(shape[aft][0] - rect['x'][0])) or
            (sign(shape[cur][0] - rect['x'][1]) != sign(shape[aft][0] - rect['x'][1]))) and
          (shape[cur][1] >= rect['y'][0]) and (shape[cur][1] <= rect['y'][1])):
          return True
      else:
        if (((sign(shape[cur][1] - rect['y'][0]) != sign(shape[aft][1] - rect['y'][0])) or
            (sign(shape[cur][1] - rect['y'][1]) != sign(shape[aft][1] - rect['y'][1]))) and
          (shape[cur][0] >= rect['x'][0]) and (shape[cur][0] <= rect['x'][1])):
          return True
      cur = cur + 1
      horiz = 1 - horiz

    return False


class Layer(IntEnum):
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


#individual rect or pin objects
class NetSeg:

  def __init__(self, layer = None, ytop = None, xleft = None):
    self.layer = layer
    self.ytop = ytop
    self.xleft = xleft
    self.map_touches = {
      Rect : self._touch_rect,
      Pin  : self._touch_pin,
      Shape : self._touch_shape,
    }

  def _touch_rect(self, net):
    pass
  def _touch_pin(self, net):
    pass
  def _touch_shape(self,net):
    pass
  def touches(self, net):
    return self.map_touches[type(net)](net)

class Pin(NetSeg):
  def __init__(self, xy, name = "", layer = Layer.LI_CONTACT, driver = False):
    super().__init__(layer = layer, ytop = xy[1], xleft = xy[0])
    self.xy = [xy] # tuple (x, y) pin center
    self.name = name
    self.driver = driver

  def _touch_rect(self, net):
    return any([net.rect['x'][0] <= n[0] and n[0] <= net.rect['x'][1] and
            net.rect['y'][0] <= n[1] and n[1] <= net.rect['y'][1] and
            (abs(self.layer - net.layer) - ((self.layer + 1) %2)) <= 1 for n in self.xy])

  def _touch_pin(self, net):
    #pin can't touch pin
    return False

  def _touch_shape(self,net):
    return point_inside_shape(net.shape, self.xy[0]) and (((abs(self.layer - net.layer) - ((self.layer +1) %2))) <= 1)

  def add_pin(self, xy):
    self.xy.append(xy)

class Rect(NetSeg):
  def __init__(self, rect, layer = Layer.LI_CONTACT):
    super().__init__(layer = layer, ytop = rect['y'][1], xleft = rect['x'][0])
    self.rect = rect # {'x': [x1, x2], 'y':[y1,y2]}

  def _touch_rect(self, net):
    return(net.rect['x'][0] <= self.rect['x'][1] and net.rect['x'][1] >= self.rect['x'][0] and
           net.rect['y'][0] <= self.rect['y'][1] and net.rect['y'][1] >= self.rect['y'][0] and
           (abs(self.layer - net.layer) - ((self.layer +1) %2)) <= 1)

  def _touch_pin(self, net):
    return any([self.rect['x'][0] < n[0] and n[0] < self.rect['x'][1] and
            self.rect['y'][0] < n[1] and n[1] < self.rect['y'][1] and
            (abs(self.layer - net.layer) - ((self.layer +1) %2)) <= 1 for n in net.xy])

  def _touch_shape(self,net):
    return shape_touch_rect(net.shape, self.rect) and ((abs(self.layer - net.layer) - ((self.layer +1) %2)) <= 1)

class Shape(NetSeg):
  def __init__(self, shape, layer = Layer.M1_ROUTE):
    ytop = -inf
    xleft = inf
    for point in shape:
      ytop = max(ytop, point[1])
      xleft = min(ytop, point[0])
    super().__init__(layer = layer, ytop = ytop, xleft = xleft)
    self.shape = shape

  def _touch_rect(self, net):
    return shape_touch_rect(self.shape, net.rect) and ((abs(self.layer - net.layer) - ((self.layer +1) %2)) <= 1)

  def _touch_pin(self, net):
    return point_inside_shape(self.shape, net.xy[0]) and ((abs(self.layer - net.layer) - ((self.layer +1) %2)) <= 1)

  def _touch_shape(self,net):
    return False #this does not occur, shapes are associated with distinct pins and hence must not collide


class Net :
  def __init__ (self, driver = None):
    self.name = ""
    self.driver = driver #this net has a driver already
    self.bound  = {"x" : [0, 0], "y":[0, 0]}   #xy bound of the net (all layers) to help with intersection test speed up
    self.segments = []   #rect objects connected to the output pin
    self.loads = []      #contact inputs in the net
    self.map_add_to_net = {
      Rect : self._add_segment,
      Pin  : self._add_pin,
      Shape : self._add_shape,
    }
    self.map_in_bound = {
      Rect : self._rect_in_bound,
      Pin  : self._pin_in_bound,
      Shape : self._shape_in_bound,
    }

  def add_to_net(self, net):
    self.map_add_to_net[type(net)](net)

  def set_name(self, name):
    self.name = name

  def has_driver(self):
    return self.driver is not None

  #rect is within the total x,y extend of the net
  def in_bound(self, net):
    return self.map_in_bound[type(net)](net)

  def _rect_in_bound(self, net):
    return(net.rect['x'][0] <= self.bound['x'][1] and net.rect['x'][1] >= self.bound['x'][0] and
           net.rect['y'][0] <= self.bound['y'][1] and net.rect['y'][1] >= self.bound['y'][0])
  def _pin_in_bound(self, net):
    return any([n[0] <= self.bound['x'][1] and n[0] >= self.bound['x'][0] and
           n[1] <= self.bound['y'][1] and n[1] >= self.bound['y'][0] for n in net.xy])

  def _shape_in_bound(self,net):
    return shape_touch_rect(net.shape, self.bound)

  def _add_segment(self, net):
    #expand bounding box and add to segment list
    self.segments.append(net)
    if (net.rect['x'][0] < self.bound['x'][0]) : self.bound['x'][0] = net.rect['x'][0]
    if (net.rect['x'][1] > self.bound['x'][1]) : self.bound['x'][1] = net.rect['x'][1]
    if (net.rect['y'][0] < self.bound['y'][0]) : self.bound['y'][0] = net.rect['y'][0]
    if (net.rect['y'][1] > self.bound['y'][1]) : self.bound['y'][1] = net.rect['y'][1]

  def _add_pin(self,net):
    self.segments.append(net)
    if(net.driver) :
      self.bound['x'][0] = min([n[0] for n in net.xy])
      self.bound['x'][1] = max([n[0] for n in net.xy])
      self.bound['y'][0] = min([n[1] for n in net.xy])
      self.bound['y'][1] = max([n[1] for n in net.xy])
    else:
      self.loads.append(net.name)
      self.bound['x'][0] = min(self.bound['x'][0], min([n[0] for n in net.xy]))
      self.bound['x'][1] = max(self.bound['x'][1], max([n[0] for n in net.xy]))
      self.bound['y'][0] = min(self.bound['y'][0], min([n[1] for n in net.xy]))
      self.bound['y'][1] = max(self.bound['y'][1], max([n[1] for n in net.xy]))

  def _add_shape(self, net):
    self.segments.append(net)
    bound = {'x': [net.shape[0][0], net.shape[0][0]], 'y':[net.shape[0][1], net.shape[0][1]]}
    for point in net.shape:
      bound['x'][0] = min(bound['x'][0], point[0])
      bound['x'][1] = max(bound['x'][1], point[0])
      bound['y'][0] = min(bound['y'][0], point[1])
      bound['y'][1] = max(bound['y'][1], point[1])
    self.bound['x'][0] = min(self.bound['x'][0], bound['x'][0])
    self.bound['x'][1] = max(self.bound['x'][1], bound['x'][1])
    self.bound['y'][0] = min(self.bound['y'][0], bound['y'][0])
    self.bound['y'][1] = max(self.bound['y'][1], bound['y'][1])


#comparator for sorting by ascending y val
def compare(item1, item2):
  ycheck = round((item1.ytop - item2.ytop)/500)
  return ycheck if ycheck != 0 else (item1.xleft - item2.xleft)

#grab the dict
in_file = open("./data/out.txt")
objects = json.load(in_file)

nets = []
#load pins and outputs, add them after whole routing structure is known
deferred_objs = []

outputs = [f'O[{i}]' for i in range(8)]
outputs.append('success')
inputs = ["I", "clk", "enable", "rst_n", "VGND", "VPWR"]

for io in objects['io'].keys():
  if io in inputs:
    pin = Pin(tuple(objects['io'][io]), name = io, layer = Layer.M3_ROUTE if io not in ["VGND", "VPWR"] else Layer.M5_ROUTE, driver = True)
    net = Net(driver = True)
    net.set_name(io)
    net.add_to_net(pin)
    nets.append(net)
  else: #output, store for later
    #if not io in ignore:
    new = Pin(objects['io'][io], name = io, layer = Layer.M3_ROUTE)
    deferred_objs.append(new)

#multiple contacts in the same pin with the LI used for routing...
multipins = {}
for pin in objects['clist']:
  if (pin['driver'] == 1):
    if pin['pinname'] not in multipins.keys():
      newpin = (Pin(tuple(pin['loc']), name = pin['pinname'], layer = Layer.M1_ROUTE, driver = True))
      multipins[pin['pinname']] = newpin
      #net = Net(driver = True)
      #net.set_name(pin['pinname'])
      #net.add_to_net(newpin)
      #nets.append(net)
    else:#add to existing
      print(f"multipin on {pin['pinname']}")
      multipins[pin['pinname']].add_pin(tuple(pin['loc']))
  else:
    if not pin['pinname'] in multipins.keys():
      newpin = Pin(tuple(pin['loc']), name = pin['pinname'], layer = Layer.M1_ROUTE)
      multipins[pin['pinname']] = newpin
      #deferred_objs.append(newpin)
    else:
      print(f"multipin on {pin['pinname']}")
      multipins[pin['pinname']].add_pin(tuple(pin['loc']))

for obj in objects['extra_objs']:
  if obj['type'] == 'pin':
    if (obj['driver'] == 1):
      if obj['pinname'] not in multipins.keys():
        newpin = (Pin(tuple(pin['loc']), name = obj['pinname'], layer = Layer.M1_ROUTE, driver = True))
        multipins[obj['pinname']] = newpin
      else:
        multipins[obj['pinname']].add_pin(tuple(obj['loc']))
    else:#defer
      if not obj['pinname'] in multipins.keys():
        newpin = Pin(tuple(obj['loc']), name = obj['pinname'], layer = Layer.M1_ROUTE)
        multipins[obj['pinname']] = newpin
      else:
        print(f"multipin on {obj['pinname']}")
        multipins[obj['pinname']].add_pin(tuple(obj['loc']))
  else:#shape, defer it
    if (cast_to_layer(obj['layer'], obj['dtype']) == None):
      continue
    deferred_objs.append(Shape(obj['coords'], layer = cast_to_layer(obj['layer'], obj['dtype'])))

for name in multipins.keys():
  if multipins[name].driver:
    net = Net(driver = True)
    net.set_name(name)
    net.add_to_net(multipins[name])
    nets.append(net)
  else:
    deferred_objs.append(multipins[name])

for shape in objects['shapes']:
  if not shape:
    continue
  #these are all rects.
  #bring them all into the list
  if (cast_to_layer(shape['layer'], shape['dtype']) == None):
    continue
  deferred_objs.append(Rect({i : shape[i] for i in ['x','y']}, layer = cast_to_layer(shape['layer'], shape['dtype'])))
  #make the net for the rect, check for intersection with any net object in nets, if so then add to that net

sort_list = sorted(deferred_objs, key=cmp_to_key(compare))

print([type(sort_list[i]) for i in range(5)])
print([sort_list[i].ytop for i in range(5)])

total_objs = len(sort_list) #for printout
total_nets = len(nets)

print(len(nets))
print(len(sort_list))


#for l in sort_list:
#  if(l._touch_pin(Pin(xy = (162149, 15469), layer = Layer.M1_ROUTE))):


direction = 1
complete_nets = []
for pass_through in range(100): # allow 20 sweeps through the object list
  if (len(nets) == 0 or len(sort_list) == 0): break;

  num_segs = [len(i.segments) for i in nets]
  to_pop = []
  for ind in range(len(sort_list))[::direction]:
    #print(round(ind/100))
    obj = sort_list[ind]
    for n in nets:
      #check if touches boundin box
      if(not n.in_bound(obj)): continue
      for seg in n.segments[::-1]: #traverse from most recent backwards
        if(seg.touches(obj)):
          n.add_to_net(obj)
          to_pop.append(ind)
          break
  new_num_segs = [len(i.segments) for i in nets]
  done_ind = [i for i in range(len(num_segs)) if num_segs[i] == new_num_segs[i]]
  complete_nets.extend([nets[i] for i in done_ind])
  nets = [nets[i] for i in range(len(num_segs)) if i not in done_ind]
  direction = direction * -1
  sort_list = [sort_list[i] for i in range(len(sort_list)) if i not in to_pop]
  print(f"Pass {pass_through}: {len(sort_list)} / {total_objs} segments remain to be allocated. {len(complete_nets)} / {total_nets} nets finished")

breakpoint()

for n in nets:
  print(len(n.segments))
#go through looking for driverless nets, when found, pick any shape in it and try to intersect with a driven net
#initial order of net addition matters a lot for how many disjoint net segments u end up with. sort by top y coord and search downwards?
  #this way if you see a top y coord that is lower than the bounding box of the whole design ... idk
#maintain a bounding box for the net segment to simplify intersection, check if the new segment intersects with bounding box first, then check each net.
#only add to driven nets, don't make new net segments.