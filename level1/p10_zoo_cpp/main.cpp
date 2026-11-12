#include <iostream>
#include <memory>
#include <stdexcept>
#include <utility>
#include <vector>

// 所有动物共用的抽象接口。
class Animal {
public:
    virtual ~Animal() = default;
    virtual void speak() const = 0;
};

class Dog : public Animal {
public:
    void speak() const override {
        std::cout << "Dog: Woof!\n";
    }
};

class Cat : public Animal {
public:
    void speak() const override {
        std::cout << "Cat: Meow!\n";
    }
};

class Zoo {
private:
    // vector 可动态增长；unique_ptr 负责动物的生命周期，避免内存泄漏。
    std::vector<std::unique_ptr<Animal>> animals;

public:
    void addAnimal(std::unique_ptr<Animal> animal) {
        if (!animal) {
            throw std::invalid_argument("Cannot add a null animal.");
        }
        animals.push_back(std::move(animal));
    }

    // 按加入顺序，让每只动物叫一次；空动物园也可以安全调用。
    void makeAllSounds() const {
        for (const auto& animal : animals) {
            animal->speak();
        }
    }
};

// 扩展功能：只添加新类，不需要修改上面的 Zoo 类。
class Bird : public Animal {
public:
    void speak() const override {
        std::cout << "Bird: Tweet!\n";
    }
};

// 狼狗仍然是一种狗，重写叫声以表现不同品种的行为。
class WolfDog : public Dog {
public:
    void speak() const override {
        std::cout << "WolfDog: Awoooo!\n";
    }
};

int main() {
    Zoo zoo;
    zoo.addAnimal(std::make_unique<Dog>());
    zoo.addAnimal(std::make_unique<Cat>());

    std::cout << "=== Part 1: Dog and Cat ===\n";
    zoo.makeAllSounds();

    zoo.addAnimal(std::make_unique<Bird>());
    zoo.addAnimal(std::make_unique<WolfDog>());
    zoo.addAnimal(std::make_unique<Dog>());

    std::cout << "\n=== Part 2: Add Bird, WolfDog and another Dog ===\n";
    zoo.makeAllSounds();

    return 0;
}
