此目录放置应用程序的实现。插件或者app可以调用它们。

其组织架构为:
      cmd    http   comp
       |      |      |
       |______|______|
              |
            core

core目录：放置core
cmd目录：放置命令行代码
http目录：放置http接口代码
comp目录: 放置组件代码
init目录: 放置初始化代码
h目录: 放置头文件
core/h目录: core内部的头文件
