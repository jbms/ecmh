from __future__ import division

import heapq

class PriorityQueue:
  def  __init__(self):  
    self.heap = []
    
  def push(self, item, priority):
      pair = (priority,item)
      heapq.heappush(self.heap,pair)

  def pop(self):
      (priority,item) = heapq.heappop(self.heap)
      return (priority,item)
  
  def isEmpty(self):
    return len(self.heap) == 0

class PriorityQueueWithFunction(PriorityQueue):
  def  __init__(self, priorityFunction, cost_bound = float('inf')):
    "priorityFunction (item) -> priority"
    self.priorityFunction = priorityFunction
    self.cost_bound = cost_bound
    PriorityQueue.__init__(self)
    
  def push(self, item):
    priority = self.priorityFunction(item, self.cost_bound)
    if priority < self.cost_bound:
      PriorityQueue.push(self, item, priority)


def genericSearch(problem, queue):
  closedSet = set()
  queue.push((problem.getStartState(), None, None, 0))
  num_expanded = 0
  path = None
  while not queue.isEmpty():
    priority, s = queue.pop();
    num_expanded += 1
    s_state, s_action, s_parent, s_cost = s
    if s_state in closedSet:
      continue
    closedSet.add(s_state)
    #print('Expanding: [%8.f : %8.f] %r' % (s_cost, priority, s_state))
    if problem.isGoalState(s_state):
      path = []
      while s_parent != None:
        path.append(s_action)
        s_state, s_action, s_parent, s_cost = s_parent
      path.reverse()
      break
    for child, action, cost in problem.getSuccessors(s_state):
      queue.push((child,action,s,cost + s_cost))
  return num_expanded, path

# Stops once the minimum cost is >= cost_bound
def genericTreeSearch(problem, queue):
  queue.push((problem.getStartState(), None, None, 0))
  num_expanded = 0
  path = None
  while not queue.isEmpty():
    priority, s = queue.pop();
    num_expanded += 1
    s_state, s_action, s_parent, s_cost = s
    if problem.isGoalState(s_state):
      path = []
      while s_parent != None:
        path.append(s_action)
        s_state, s_action, s_parent, s_cost = s_parent
      path.reverse()
      break
    for child, action, cost in problem.getSuccessors(s_state):
      queue.push((child,action,s,cost + s_cost))
  return num_expanded, path
   

def uniformCostSearch(problem):
    return genericSearch(problem, PriorityQueueWithFunction(lambda x: x[3]))

def nullHeuristic(state, problem=None):
  return 0

better_bound = False
def aStarSearch(problem, heuristic=nullHeuristic, cost_bound = float('inf')):
  if better_bound:
    def priorityFunction(x, bound):
      h1 = heuristic(x[0],problem,bound - x[3])
      h2 = heuristic(x[0],problem,bound)
      if h1 != h2:
        assert(h1 == bound - x[3] and h2 > h1 and h2 < bound)
        assert(x[3] + h1 >= bound)
        assert(x[3] + h2 >= bound)
      return x[3] + h1

  else:
    def priorityFunction(x, bound):
      return x[3] + heuristic(x[0],problem,bound)
  return genericSearch(problem, PriorityQueueWithFunction(priorityFunction, cost_bound))

def aStarTreeSearch(problem, heuristic=nullHeuristic, cost_bound = float('inf')):
  if better_bound:
    def priorityFunction(x, bound):
      return x[3] + heuristic(x[0],problem,bound - x[3])
  else:
    def priorityFunction(x, bound):
      return x[3] + heuristic(x[0],problem,bound)
  return genericTreeSearch(problem, PriorityQueueWithFunction(priorityFunction, cost_bound))
