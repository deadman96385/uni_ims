#!/usr/bin/env python2

# Copyright (C) 2009,2010  Xyne
#
# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License
# (version 2) as published by the Free Software Foundation.
#
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.

import os.path, socket, ssl
import BaseHTTPServer
from SocketServer import ThreadingMixIn
from hashlib import md5
from mimetypes import guess_type
from random import randint
from time import time



class ThreadedHTTPServer(ThreadingMixIn, BaseHTTPServer.HTTPServer):

  # Convenience method for running the server.
  def run_server(self):
    try:
      while self.keep_serving:
        self.handle_request()

      self.shutdown()
      message = "exiting... "

    except KeyboardInterrupt:
      self.socket.close()
      message = '<Ctrl-C> pressed, exiting...'

    except socket.error as e:
      message = "error: " + str(e.strerror)

    return message



  # Override to enable SSL
  def get_request(self):
    conn, addr = self.socket.accept()

    if self.use_ssl:
      conn = ssl.wrap_socket(conn, **self.ssl_parameters)

    return conn, addr



  def __init__(self, server_address, HandlerClass, username=None, password=None, authfile=None, use_ssl=False, ssl_parameters = {}, cert_required=False):
      BaseHTTPServer.HTTPServer.__init__(self, server_address, HandlerClass)

      self.daemon_threads = True
      self.keep_serving = True

      if authfile:
        f = open(authfile, 'r')
        self.username = f.readline().rstrip('\r\n\0')
        self.password = f.readline().rstrip('\r\n\0')
        f.close()
      else:
        self.username = username
        self.password = password

      self.use_ssl = use_ssl


      self.ssl_parameters = ssl_parameters
      self.ssl_parameters['server_side'] = True
      if cert_required:
        self.ssl_parameters['cert_reqs'] = ssl.CERT_REQUIRED
        if not self.ssl_parameters['ca_certs']:
          self.ssl_parameters['ca_certs'] = self.ssl_parameters['certfile']



  # Convenience method for display some useful data at startup.
  def display_status(self):
    address, port = self.server_address

    if not address or address == '0.0.0.0':
      print "  address: all interfaces"
    else:
      print "  address: " + address
    print "  port: " + str(port)
    print "  pid: " + str(os.getpid())

    if self.username and self.password:
      print "  authentication"
      print "    username: " + self.username
      print "    password: " + self.password

    if self.use_ssl:
      print "  SSL enabled"
      if self.ssl_parameters['certfile']:
        print "    certificate: " + self.ssl_parameters['certfile']
      if self.ssl_parameters['keyfile']:
        print "    key: " + self.ssl_parameters['keyfile']











class BaseHTTPRequestHandler(BaseHTTPServer.BaseHTTPRequestHandler):

  ##### HTTP Authentication #####

  # nc limit: maximum value is 0xffffffff
  # The client must re-auth after this number of requests.
  NC_LIMIT = 0xffffffff

  # Client must re-auth if no requests were made in the last x seconds
  OPAQUE_TIMEOUT = 3600

  # nonce and opaque lengths
  NONCE_LENGTH = 32

  # Opaque values
  opaques = {}



  # Create a nonce value
  def create_nonce(self):
    NONCE_MAX = 2 ** (self.NONCE_LENGTH * 4) - 1
    NONCE_FORMAT = '%0' + str(self.NONCE_LENGTH) + 'x'
    return NONCE_FORMAT % randint(0, NONCE_MAX)




  # Get the nonce value
  def get_nonce(self,opaque,nc):
    # Purge values that have timed out or exceeded the limit
    t = time()
    for k in self.opaques.keys():
     if t - self.opaques[k]['time'] > self.OPAQUE_TIMEOUT or int(self.opaques[k]['nc'],16) > self.NC_LIMIT:
       del self.opaques[k]

    try:
      if self.opaques[opaque]['nc'] != nc:
        del self.opaques[opaque]
        return ''

      self.opaques[opaque]['nc'] = "%08x" % (int(nc,16) + 1)
      self.opaques[opaque]['time'] = t
      return self.opaques[opaque]['nonce']
    except KeyError:
      return None



  # Reject an unauthenticated request.
  def reject_unauthenticated_request(self):
    nonce =  self.create_nonce()
    opaque = self.create_nonce()
    self.opaques[opaque] = {'time': time(), 'nc': '00000001', 'nonce': nonce}
    self.send_response(401)
    self.send_header('Content-Length', 0)
    self.send_header('WWW-Authenticate', 'Digest realm="' + self.address_string() + '",qop="auth",nonce="' + nonce +'",opaque="' + opaque +'"')
    self.send_header('Connection', 'close')
    self.end_headers()



  # Attempt to authenticate requests and reject those which fail.
  # Accepts the method (e.g. "GET", "POST") and returns True if the request
  # could be authenticated, otherwise False
  def authenticate(self, method='GET'):
    username = self.server.username
    password = self.server.password
    
    if username and password:
      if 'Authorization' in self.headers and self.headers['Authorization'][:7] == 'Digest ':
        nc = None
        cnonce = None
        opaque = None
        client_response = None

        for field in self.headers['Authorization'][7:].split(','):
          field = field.strip()
          if field[:10] == 'response="':
            client_response = field[10:-1]
          elif field[:8] == 'cnonce="':
            cnonce = field[8:-1]
          elif field[:3] == 'nc=':
            nc = field[3:]
          elif field[:8] == 'opaque="':
            opaque = field[8:-1]

        if client_response != None and cnonce != None and opaque != None and nc != None:
          nonce = self.get_nonce(opaque,nc)
          if not nonce:
            self.reject_unauthenticated_request()
            return False

          m = md5()
          m.update(username + ':' + self.address_string() + ':' + password)
          ha1 = m.hexdigest()

          m = md5()
          m.update(method + ':' + self.path)
          ha2 = m.hexdigest()

          m = md5()
          m.update(ha1 + ':' + nonce + ':' + nc + ':' + cnonce + ':auth:' + ha2)
          response = m.hexdigest()

          print "my resp:"+response+" client resp:"+client_response
          if response == client_response:
            return True

      self.reject_unauthenticated_request()
      return False

    elif not username and not password:
      return True

    else:
      self.reject_unauthenticated_request()
      return False




  ##### Meta #####

  # HTTP protocol version
  protocol_version = 'HTTP/1.1'

  # Server version.
  def version_string(self):
    return 'XyneHTTPServer/1.0'




  ##### Convencience Methods #####

  # Convenience method for transfering content.
  def transfer_utf8_content(self, body, content_type, include_body=True, response_code=200):
    try:
      body = body.encode('utf-8')
    except UnicodeDecodeError:
      pass

    content_type += '; charset=UTF-8'

    try:
      self.send_response(response_code)
      self.send_header('Content-Type', content_type)
      self.send_header('Content-Length', len(body))
      self.end_headers()
      if include_body:
        self.wfile.write(body)
    except:
      pass



  # Convencience method for serving UTF-8 encoded HTML pages.
  def transfer_html(self, html, include_body=True, response_code=200):
    self.transfer_utf8_content(html, 'text/html', include_body)



  # Convencience method for serving UTF-8 encoded plaintext pages.
  def transfer_plaintext(self, text, include_body=True, response_code=200):
    self.transfer_utf8_content(text, 'text/plain', include_body)



  # Convenience method for transfering a file.
  def transfer_file(self,fpath):
    self.send_response(200)
    size = os.path.getsize(fpath)
    range_start = 0
    range_end = size
    mimetype, encoding = guess_type(fpath)
    if not mimetype:
      mimetype = 'application/octet-stream'
    self.send_header('Content-Type', mimetype)

    if encoding:
      self.send_header('Content-Encoding', encoding)

    self.send_header('Accept-Ranges', 'bytes')

    if 'Range' in self.headers:
      s, e = self.headers['range'][6:].split('-', 1)
      sl = len(s)
      el = len(e)
      if sl > 0:
        range_start = int(s)
        if el > 0:
          range_end = int(e) + 1
      elif el > 0:
        ei = int(e)
        if ei < size:
          range_start = size - ei

    self.send_header('Content-Range', 'bytes ' + str(range_start) + '-' + str(range_end - 1) + '/' + str(size))
    self.send_header('Content-Length', range_end - range_start)
    self.end_headers()

    f = open(fpath, 'rb')
    f.seek(range_start, 0)
    step = 0x8000
    total = 0
    while step > 0:
      if range_start + step > range_end:
        step = range_end - range_start
      try:
        self.wfile.write( f.read(step) )
      except:
        break
      total += step
      range_start += step
    f.close()





  def local_redirect(self, path='/', response_code=303, message=None):
    self.send_response(response_code, message)
#     address, port = self.request.getsockname()
#     if self.server.use_ssl:
#       scheme = 'https'
#     else:
#       scheme = 'http'
#     url = '%s://%s:%s%s' % (scheme, address, port, path)
    url = path
#    self.send_header('Content-Type', 'text/html')
    self.send_header('Content-Length', 0)
#    self.send_header('Content-Location', url)
    self.send_header('Location', url)
#    self.send_header('Connection', 'close')
    self.end_headers()
    self.wfile.write('')






  ##### Request Handlers #####

  def do_GET(self):
    if self.authenticate('GET'):
      self.do_authenticated_GET()

  def do_authenticated_GET(self):
    BaseHTTPServer.BaseHTTPRequestHandler.do_GET(self)

  def do_PUT(self):
    if self.authenticate('PUT'):
      self.do_authenticated_PUT()

  def do_authenticated_PUT(self):
    self.send_error(501, "Unsupported method PUT in Xyne")


  def do_HEAD(self):
    if self.authenticate('GET'):
      self.do_authenticated_HEAD()

  def do_authenticated_HEAD(self):
    BaseHTTPServer.BaseHTTPRequestHandler.do_HEAD(self)



  def do_POST(self):
    if self.authenticate('POST'):
      self.do_authenticated_POST()
    return True

  def do_authenticated_POST(self):
    BaseHTTPServer.BaseHTTPRequestHandler.do_POST(self)












