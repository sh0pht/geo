#include <QtWidgets>
#include <QOpenGLWidget>
#include <cmath>
#include <fstream>
#include <vector>
struct Segment { QPointF a,b; QColor color; };
class CadView : public QOpenGLWidget {
public:
 std::vector<Segment> segments; double scale=1.0; QPointF offset{0,0}; QPoint last; bool panning=false;
 explicit CadView(QWidget *p=nullptr):QOpenGLWidget(p){setMouseTracking(true);setFocusPolicy(Qt::StrongFocus);}
 QPointF screen(QPointF p) const {return QPointF(width()/2.0+offset.x()+p.x()*scale,height()/2.0+offset.y()-p.y()*scale);}
 QPointF world(QPointF p) const {return QPointF((p.x()-width()/2.0-offset.x())/scale,(height()/2.0+offset.y()-p.y())/scale);}
 void fit(){if(segments.empty()){scale=1;offset={0,0};update();return;} QRectF r; bool first=true;for(auto&s:segments){for(auto p:{s.a,s.b}){if(first){r=QRectF(p,QSizeF(0,0));first=false;}else r|=QRectF(p,QSizeF(0,0));}}scale=std::min((width()-80)/std::max(1.0,r.width()),(height()-80)/std::max(1.0,r.height()));scale=std::max(0.00001,scale);offset=QPointF(-r.center().x()*scale,r.center().y()*scale);update();}
protected:
 void paintGL() override {QPainter p(this);p.fillRect(rect(),QColor(24,28,34));p.setRenderHint(QPainter::Antialiasing,false);p.setPen(QPen(QColor(45,51,59),1));double grid=100;while(grid*scale<25)grid*=10;while(grid*scale>250)grid/=10;QPointF a=world(QPointF(0,height())),b=world(QPointF(width(),0));int n=0;for(double x=std::floor(a.x()/grid)*grid;x<=b.x()&&n++<300;x+=grid){double sx=screen(QPointF(x,0)).x();p.drawLine(QPointF(sx,0),QPointF(sx,height()));}n=0;for(double y=std::floor(a.y()/grid)*grid;y<=b.y()&&n++<300;y+=grid){double sy=screen(QPointF(0,y)).y();p.drawLine(QPointF(0,sy),QPointF(width(),sy));}p.setPen(QPen(QColor(80,120,160),1));p.drawLine(screen({0,0}),screen({100/scale,0}));p.drawLine(screen({0,0}),screen({0,100/scale}));for(auto&s:segments){p.setPen(QPen(s.color,1));p.drawLine(screen(s.a),screen(s.b));}p.setPen(Qt::white);p.drawText(12,20,QString("MODEL | %1 entities | zoom %2").arg(segments.size()).arg(scale,0,'g',3));}
 void wheelEvent(QWheelEvent *e) override {QPointF before=world(e->position());scale*=std::pow(1.15,e->angleDelta().y()/120.0);scale=std::clamp(scale,1e-7,1e7);QPointF after=screen(before);offset+=e->position()-after;update();}
 void mousePressEvent(QMouseEvent *e) override {if(e->button()==Qt::MiddleButton||e->button()==Qt::RightButton){panning=true;last=e->pos();setCursor(Qt::ClosedHandCursor);}}
 void mouseMoveEvent(QMouseEvent *e) override {if(panning){offset+=e->pos()-last;last=e->pos();update();}else{setToolTip(QString("X: %1  Y: %2").arg(world(e->position()).x(),0,'f',3).arg(world(e->position()).y(),0,'f',3));}}
 void mouseReleaseEvent(QMouseEvent *) override {panning=false;unsetCursor();}
};
// Minimal ASCII DXF LINE reader; deliberately does not claim full DXF support.
static bool loadDxf(const QString &path,std::vector<Segment>&out,QString &error){QFile f(path);if(!f.open(QIODevice::ReadOnly|QIODevice::Text)){error=f.errorString();return false;}QTextStream in(&f);QStringList lines;while(!in.atEnd())lines<<in.readLine();if(lines.size()>2000000){error="DXF too large for this initial parser";return false;}QString entity;double x1=0,y1=0,x2=0,y2=0;bool has10=false,has20=false,has11=false,has21=false;auto commit=[&](){if(entity=="LINE"&&has10&&has20&&has11&&has21)out.push_back({{x1,y1},{x2,y2},QColor(235,235,235)});};for(int i=0;i+1<lines.size();i+=2){QString code=lines[i].trimmed(),val=lines[i+1].trimmed();if(code=="0"){commit();entity=val;has10=has20=has11=has21=false;}else if(entity=="LINE"){bool ok=false;double v=val.toDouble(&ok);if(!ok)continue;if(code=="10"){x1=v;has10=true;}if(code=="20"){y1=v;has20=true;}if(code=="11"){x2=v;has11=true;}if(code=="21"){y2=v;has21=true;}}}commit();return true;}
int main(int argc,char**argv){
 QApplication app(argc,argv);app.setOrganizationName("GeoCAD");app.setApplicationName("GeoCAD");
 QMainWindow win;win.setObjectName("GeoCADMainWindow");win.setWindowTitle("GeoCAD 0.2 — Classic / Ribbon");win.resize(1440,900);
 win.setDockOptions(QMainWindow::AllowNestedDocks|QMainWindow::AllowTabbedDocks|QMainWindow::AnimatedDocks);
 win.setStyleSheet("QMainWindow,QMenuBar,QMenu,QToolBar,QDockWidget,QStatusBar{background:#303842;color:#e3e8ee;} QLineEdit,QListWidget{background:#202830;color:#e3e8ee;border:1px solid #56616c;} QToolButton{padding:3px;} QToolButton:hover{background:#536474;}");
 auto*view=new CadView;win.setCentralWidget(view);
 auto*fileMenu=win.menuBar()->addMenu("Файл");auto*editMenu=win.menuBar()->addMenu("Правка");auto*viewMenu=win.menuBar()->addMenu("Вид");auto*drawMenu=win.menuBar()->addMenu("Рисование");auto*modifyMenu=win.menuBar()->addMenu("Редактирование");auto*toolsMenu=win.menuBar()->addMenu("Сервис");auto*workspaceMenu=win.menuBar()->addMenu("Рабочее пространство");
 auto*open=fileMenu->addAction("Открыть DXF...");open->setShortcut(QKeySequence::Open);
 QObject::connect(open,&QAction::triggered,[&](){QString path=QFileDialog::getOpenFileName(&win,"Открыть ASCII DXF",{},"DXF (*.dxf)");if(path.isEmpty())return;std::vector<Segment> loaded;QString error;if(!loadDxf(path,loaded,error)){QMessageBox::warning(&win,"Ошибка",error);return;}view->segments=std::move(loaded);view->fit();win.statusBar()->showMessage(QString("Загружено LINE: %1. Остальные объекты пока не поддерживаются.").arg(view->segments.size()));});
 auto*fit=viewMenu->addAction("Показать всё");QObject::connect(fit,&QAction::triggered,[&](){view->fit();});
 auto*reset=viewMenu->addAction("Сбросить вид");QObject::connect(reset,&QAction::triggered,[&](){view->scale=1;view->offset={0,0};view->update();});
 QList<QToolBar*> classic;
 auto makeBar=[&](const QString&name){auto*b=new QToolBar(name,&win);b->setObjectName("classic_"+name);b->setMovable(true);b->setFloatable(true);win.addToolBar(Qt::TopToolBarArea,b);classic<<b;viewMenu->addAction(b->toggleViewAction());return b;};
 auto*standard=makeBar("Стандартная");standard->addAction(open);standard->addAction(fit);standard->addAction(reset);
 auto*draw=makeBar("Рисование");auto*modify=makeBar("Редактирование");auto*annotate=makeBar("Аннотации");auto*layers=makeBar("Слои");
 auto unavailable=[&](QToolBar*b,QMenu*m,const QStringList&names){for(const auto&n:names){auto*a=b->addAction(n);m->addAction(a);QObject::connect(a,&QAction::triggered,[&,n](){win.statusBar()->showMessage("Инструмент в плане разработки: "+n,5000);});}};
 unavailable(draw,drawMenu,{"Линия","Полилиния","Окружность","Дуга","Прямоугольник"});
 unavailable(modify,modifyMenu,{"Перенести","Копировать","Обрезать","Сместить","Повернуть"});
 unavailable(annotate,toolsMenu,{"Текст","Размер","Выноска","Отметка"});
 unavailable(layers,toolsMenu,{"Новый слой","Свойства слоя"});
 auto*dock=new QDockWidget("Слои",&win);dock->setObjectName("dock_layers");auto*list=new QListWidget;list->addItem("0  (просмотр; без редактирования)");dock->setWidget(list);win.addDockWidget(Qt::LeftDockWidgetArea,dock);viewMenu->addAction(dock->toggleViewAction());
 auto*command=new QDockWidget("Командная строка",&win);command->setObjectName("dock_command");auto*input=new QLineEdit;input->setPlaceholderText("Введите команду (ZE / ZOOM EXTENTS)");QObject::connect(input,&QLineEdit::returnPressed,[&](){auto cmd=input->text().trimmed().toUpper();if(cmd=="ZOOM EXTENTS"||cmd=="_ZE"||cmd=="ZE")view->fit();else win.statusBar()->showMessage("Команда пока не реализована: "+cmd,5000);input->clear();});command->setWidget(input);win.addDockWidget(Qt::BottomDockWidgetArea,command);viewMenu->addAction(command->toggleViewAction());
 auto*ribbon=new QToolBar("Лента",&win);ribbon->setObjectName("ribbon_toolbar");ribbon->setMovable(false);win.addToolBar(Qt::TopToolBarArea,ribbon);
 auto*ribbonTabs=new QTabWidget(ribbon);ribbonTabs->setDocumentMode(true);ribbonTabs->setMinimumHeight(100);
 auto addTab=[&](const QString&name,const QList<QAction*>&actions){auto*page=new QWidget;auto*layout=new QHBoxLayout(page);layout->setContentsMargins(8,4,8,4);for(auto*a:actions){auto*button=new QToolButton(page);button->setDefaultAction(a);button->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);button->setMinimumWidth(75);layout->addWidget(button);}layout->addStretch();ribbonTabs->addTab(page,name);};
 addTab("Главная",{open,fit,draw->actions().at(0),draw->actions().at(1),modify->actions().at(0),modify->actions().at(1)});
 addTab("Рисование",draw->actions());addTab("Редактирование",modify->actions());addTab("Аннотации",annotate->actions());
 ribbon->addWidget(ribbonTabs);
 QSettings settings;bool isRibbon=settings.value("workspace/ribbon",false).toBool();
 auto applyWorkspace=[&](bool useRibbon){isRibbon=useRibbon;ribbon->setVisible(useRibbon);for(auto*b:classic)b->setVisible(!useRibbon);settings.setValue("workspace/ribbon",useRibbon);};
 auto*classicAction=workspaceMenu->addAction("Классический интерфейс");auto*ribbonAction=workspaceMenu->addAction("Ленточный интерфейс");
 QObject::connect(classicAction,&QAction::triggered,[&](){applyWorkspace(false);});QObject::connect(ribbonAction,&QAction::triggered,[&](){applyWorkspace(true);});
 auto*saveWorkspace=workspaceMenu->addAction("Сохранить расположение панелей");QObject::connect(saveWorkspace,&QAction::triggered,[&](){settings.setValue("window/geometry",win.saveGeometry());settings.setValue("window/state",win.saveState());win.statusBar()->showMessage("Рабочее пространство сохранено",3000);});
 auto*restoreWorkspace=workspaceMenu->addAction("Сбросить расположение панелей");QObject::connect(restoreWorkspace,&QAction::triggered,[&](){settings.remove("window/state");win.statusBar()->showMessage("Настройки расположения очищены; перезапустите GeoCAD",5000);});
 if(settings.contains("window/geometry"))win.restoreGeometry(settings.value("window/geometry").toByteArray());
 if(settings.contains("window/state"))win.restoreState(settings.value("window/state").toByteArray());
 applyWorkspace(isRibbon);
 QObject::connect(&app,&QCoreApplication::aboutToQuit,[&](){settings.setValue("window/geometry",win.saveGeometry());settings.setValue("window/state",win.saveState());});
 win.statusBar()->showMessage("GeoCAD 0.2 | Колесо: масштаб | Средняя кнопка: панорама | DWG и облака точек пока не реализованы");win.show();return app.exec();
}
