#include <iostream>
#include <QObject>
#include <QVariant>
#include <QMetaProperty>

class MyClass : public QObject
 {
     Q_OBJECT
     Q_PROPERTY(Priority priority READ priority WRITE setPriority NOTIFY priorityChanged)
     Q_ENUMS(Priority)
     Q_CLASSINFO("Version", "3.0.0")
 public:
     MyClass() {}
     //MyClass(QObject *parent = 0);
     ~MyClass();

     enum Priority { High, Low, VeryHigh, VeryLow };

     void setPriority(Priority priority)
     {
         m_priority = priority;
         emit priorityChanged(priority);
     }
     Priority priority() const
     { return m_priority; }

 signals:
     void priorityChanged(Priority);

 private:
     Priority m_priority;
 };


void MyClass::priorityChanged(Priority)
{
    // pass
}
void dump (QObject *object)
{
    std::cout << "object: " << std::endl;
    const QMetaObject *metaobject = object->metaObject();
    int count = metaobject->propertyCount();
    for (int i=0; i<count; ++i)
    {
      QMetaProperty metaproperty = metaobject->property(i);
      const char *name = metaproperty.name();
      QVariant value = object->property(name);
      std::cout << " * " << name << " = " << value.toString().toStdString() << std::endl;
    }
}

int main(int /* argc */, char ** /* argv */)
{
  MyClass *myinstance = new MyClass();
  //QObject *object = myinstance;

  myinstance->setPriority(MyClass::VeryHigh);
  //object->setProperty("priority", "VeryHigh");

  dump ((QObject *) myinstance);

  delete myinstance;

  return 0;
}
