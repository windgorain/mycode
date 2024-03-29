(function(factory) {

  if (typeof define == 'function') {
    define('#cookiejs', [], factory);
  }
  else {
    factory();
  }

})(function(require) {

// Copyright (c) 2012 Florian H., https://github.com/js-coder https://github.com/js-coder/cookie.js
!function(a,b){"use strict";var c={isArray:Array.isArray||function(a){return Object.prototype.toString.call(a)==="[object Array]"},isPlainObject:function(a){return Object.prototype.toString.call(a)==="[object Object]"},toArray:function(a){return Array.prototype.slice.call(a)},getKeys:Object.keys||function(a){var b=[],c="";for(c in a)a.hasOwnProperty(c)&&b.push(c);return b},escape:function(a){return a.replace(/[,;"\\=\s%]/g,function(a){return encodeURIComponent(a)})},retrieve:function(a,c){return a===b?c:a}},d=function(){return d.get.apply(d,arguments)};d.defaults={},d.expiresMultiplier=86400,d.set=function(d,e,f){if(c.isPlainObject(d))for(var g in d)d.hasOwnProperty(g)&&this.set(g,d[g],e);else{f=c.isPlainObject(f)?f:{expires:f};var h=f.expires!==b?f.expires:this.defaults.expires||"",i=typeof h;i==="string"&&h!==""?h=new Date(h):i==="number"&&(h=new Date(+(new Date)+1e3*this.expiresMultiplier*h)),h!==""&&"toGMTString"in h&&(h=";expires="+h.toGMTString());var j=f.path||this.defaults.path;j=j?";path="+j:"";var k=f.domain||this.defaults.domain;k=k?";domain="+k:"";var l=f.secure||this.defaults.secure?";secure":"";a.cookie=c.escape(d)+"="+c.escape(e)+h+j+k+l}return this},d.remove=function(a){a=c.isArray(a)?a:c.toArray(arguments);for(var b=0,d=a.length;b<d;b++)this.set(a[b],"",-1);return this},d.empty=function(){return this.remove(c.getKeys(this.all()))},d.get=function(a,d){d=d||b;var e=this.all();if(c.isArray(a)){var f={};for(var g=0,h=a.length;g<h;g++){var i=a[g];f[i]=c.retrieve(e[i],d)}return f}return c.retrieve(e[a],d)},d.all=function(){if(a.cookie==="")return{};var b=a.cookie.split("; "),c={};for(var d=0,e=b.length;d<e;d++){var f=b[d].split("=");c[decodeURIComponent(f[0])]=decodeURIComponent(f[1])}return c},d.enabled=function(){if(navigator.cookieEnabled)return!0;var a=d.set("_","_").get("_")==="_";return d.remove("a"),a},typeof define=="function"&&define.amd?define(function(){return d}):typeof exports!="undefined"?exports.cookie=d:window.cookie=d}(document);


});
