var vock = require('../vock'),
    util = require('util'),
    dgram = require('dgram'),
    msgpack = require('msgpack-js'),
    EventEmitter = require('events').EventEmitter;

var server = exports;

function Server(options) {
  EventEmitter.call(this);
  if (!options) options = {};

  this.target = null;
  this.rtarget = null;
  this.protocol = vock.protocol.create();

  // Create audio unit
  this.audio = vock.audio.create(48000);

  this.socket = dgram.createSocket('udp4');

  this.init(options);
};
util.inherits(Server, EventEmitter);

server.create = function create(options) {
  return new Server(options);
};

Server.prototype.init = function init(options) {
  var self = this,
      lastRinfo;

  // Pass packets from other side to protocol parser
  this.socket.on('message', function(raw, rinfo) {
    if (!self.target) return;
    if (self.rtarget &&
        (self.rtarget.address !== rinfo.address ||
         self.rtarget.port !== rinfo.port)) {
      // Connection came from different address
      return;
    }
    lastRinfo = rinfo;

    try {
      var packet = msgpack.decode(raw);
      self.protocol.receive(packet);
    } catch (e) {
      self.emit('error', e);
    }
  });

  if (!options.mute) {
    this.audio.on('data', function(data) {
      self.protocol.sendVoice(data);
    });
  }

  // Pass packets from protocol to other side
  this.protocol.on('data', function(packet) {
    if (!self.target) return;

    var buf = msgpack.encode(packet);
    self.socket.send(buf, 0, buf.length, self.target.port, self.target.host);
  });

  this.protocol.on('connect', function() {
    self.rtarget = lastRinfo;
    self.audio.start();
  });

  this.protocol.on('close', function() {
    self.audio.stop();
  });

  // Play received data
  this.protocol.on('voice', function(data) {
    self.audio.play(data);
  });

  // Propogate errors from protocol to
  this.protocol.on('error', function(err) {
    self.emit('error', err);
  });
};

Server.prototype.listen = function listen(port, host, callback) {
  var self = this;

  this.socket.once('listening', function() {
    callback && callback.call(self);
  });
  this.socket.bind(port, host);
};

Server.prototype.connect = function connect(port, host, callback) {
  var self = this;

  this.target = {
    port: port,
    host: host
  };
  this.protocol.connect();
  this.protocol.once('connect', function() {
    callback && callback.call(self);
  });
};
