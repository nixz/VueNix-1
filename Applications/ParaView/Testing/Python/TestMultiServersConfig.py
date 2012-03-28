from paraview.simple import *
from paraview import servermanager

import time

# Make sure the test driver know that process has properly started
print "Process started"
errors = 0

#-------------------- Helpers methods ----------------
def getHost(url):
   return url.split(':')[1][2:]

def getScheme(url):
   return url.split(':')[0]

def getPort(url):
   return int(url.split(':')[2])
#--------------------
import os

def findInSubdirectory(filename, subdirectory=''):
    if subdirectory:
        path = subdirectory
    else:
        path = os.getcwd()
    for root, dirs, names in os.walk(path):
        for name in names:
           if (name.find(filename) > -1) and ( (name.find('.dll') > -1) or (name.find('.so') > -1) or (name.find('.dylib') > -1)):
              return os.path.join(root, name)
    raise 'File not found'
#--------------------

print "Start multi-server testing"
enableMultiServer()

options = servermanager.vtkProcessModule.GetProcessModule().GetOptions()
available_server_urls = options.GetServerURL().split('|')
built_in_connection = servermanager.ActiveConnection

# Search plugin path
plugin_path = findInSubdirectory('PacMan', options.GetApplicationPath()[0:-12]);
print "====> Found plugin path: ", plugin_path

# Test if the built-in connection is here
if (len(servermanager.MultiServerConnections) != 1):
  errors += 1
  print "Error pvpython should be connected to a built-in session. Currently connected to ", servermanager.MultiServerConnections

url = available_server_urls[0]
print "Connect to first server ", url
server1_connection = Connect(getHost(url), getPort(url))

# Test that we have one more connection
if (len(servermanager.MultiServerConnections) != 2):
  errors += 1
  print "Error pvpython should be connected to a built-in session + one remote one. Currently connected to ", servermanager.MultiServerConnections

url = available_server_urls[1]
print "Connect to second server ", url
server2_connection = Connect(getHost(url), getPort(url))

# Test that we have one more connection
if (len(servermanager.MultiServerConnections) != 3):
  errors += 1
  print "Error pvpython should be connected to a built-in session + two remote one. Currently connected to ", servermanager.MultiServerConnections

print "Available connections: ", servermanager.MultiServerConnections

# Test that last created connection is the active one
if ( servermanager.ActiveConnection != server2_connection):
  errors += 1
  print "Error Invalid active connection. Expected ", server2_connection, " and got ", servermanager.ActiveConnection

# Test that switchActiveConnection is working as expected
switchActiveConnection(server1_connection, globals())
if ( servermanager.ActiveConnection != server1_connection):
  errors += 1
  print "Error Invalid active connection. Expected ", server1_connection, " and got ", servermanager.ActiveConnection

# Test that switchActiveConnection is working as expected
switchActiveConnection(built_in_connection, globals())
if ( servermanager.ActiveConnection != built_in_connection):
  errors += 1
  print "Error Invalid active connection. Expected ", built_in_connection, " and got ", servermanager.ActiveConnection

# Test that switchActiveConnection is working as expected
switchActiveConnection(server2_connection, globals())
if ( servermanager.ActiveConnection != server2_connection):
  errors += 1
  print "Error Invalid active connection. Expected ", server2_connection, " and got ", servermanager.ActiveConnection


# Load plugin on server2
switchActiveConnection(server2_connection, globals())
LoadPlugin(plugin_path, True, globals())

# Create PacMan on server2
pacMan_s2 = PacMan()

# Swtich to server1 and Create PacMan ==> This should fail
switchActiveConnection(server1_connection, globals())
try:
  pacMan_s1 = PacMan()
  errors += 1
  print "Error: PacMan should not be available on Server1"
except NameError:
  print "OK: PacMan is not available on server1"

# Swtich to server2 with globals and switch back to server1 with not updating the globals
switchActiveConnection(server2_connection, globals())
switchActiveConnection(server1_connection)

# Create PacMan ==> This should fail
try:
  pacMan_s1 = PacMan()
  errors += 1
  print "Error: PacMan should not be available on Server1"
except RuntimeError:
  print "OK: PacMan is not available on server1"

# Make sure built-in as not the pacMan
switchActiveConnection(server2_connection, globals())
switchActiveConnection(built_in_connection, globals())
try:
  pacMan_builtin = PacMan()
  errors += 1
  print "Error: PacMan should not be available on built-in"
except NameError:
  print "OK: PacMan is not available on built-in"

# Load plugin localy for built-in
# Create PacMan ==> This should be OK on built-in
switchActiveConnection(built_in_connection, globals())
LoadPlugin(plugin_path, False, globals())
pacMan_builtin = PacMan()
print "After loading the plugin locally in built-in, the PacMan definition is available"

# Swtich to server1 and Create PacMan ==> This should fail
switchActiveConnection(server1_connection, globals())
try:
  pacMan_s1 = PacMan()
  errors += 1
  print "Error: PacMan should not be available on Server1"
except NameError:
  print "OK: PacMan is still not available on server1"

# Disconnect and quit application...
Disconnect()
print "Available connections after disconnect: ", servermanager.MultiServerConnections
Disconnect()
print "Available connections after disconnect: ", servermanager.MultiServerConnections
Disconnect()
print "Available connections after disconnect: ", servermanager.MultiServerConnections

if errors > 0:
  raise RuntimeError, "An error occured during the execution"
