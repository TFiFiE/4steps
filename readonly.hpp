#ifndef READONLY_HPP
#define READONLY_HPP

template<class Class,typename Type>
class readonly {
  friend Class;
private:
  Type data;

  readonly() {}
  readonly(const Type& data_) : data(data_) {}
  Type operator=(const Type& data_)
  {
    data=data_;
    return data;
  }
public:
  operator const Type&() const
  {
    return data;
  }
  const Type& get() const
  {
    return data;
  }
};

#endif // READONLY_HPP
