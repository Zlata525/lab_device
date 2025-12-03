/**
 * @file device.cpp
 * @brief Лабораторная работа 1.
 */

#include <iostream>
#include <string>
#include <vector>
#include <memory>
#include <cmath>

using namespace std;

int streamcounter;

const int MIXER_OUTPUTS = 1;
const float POSSIBLE_ERROR = 0.01;

class Device; 

/**
 * @class Stream
 * @brief Поток вещества + weak_ptr на аппараты
 */
class Stream
{
private:
    double mass_flow;
    string name;

    weak_ptr<Device> sourceDevice;  ///< из какого аппарата вытекает
    weak_ptr<Device> targetDevice;  ///< в какой аппарат втекает

public:
    Stream(int s){ setName("s" + to_string(s)); }

    void setName(const string& s){ name = s; }
    string getName() const { return name; }

    void setMassFlow(double m){ mass_flow = m; }
    double getMassFlow() const { return mass_flow; }

    void setSourceDevice(shared_ptr<Device> d){ sourceDevice = d; }
    void setTargetDevice(shared_ptr<Device> d){ targetDevice = d; }

    shared_ptr<Device> getSourceDevice(){ return sourceDevice.lock(); }
    shared_ptr<Device> getTargetDevice(){ return targetDevice.lock(); }

    void print(){
        cout << "Stream " << getName() << " flow = " << getMassFlow() << endl;
    }
};

/**
 * @class Device
 * @brief Базовый аппарат.
 */
class Device : public enable_shared_from_this<Device>
{
protected:
    vector<shared_ptr<Stream>> inputs;
    vector<shared_ptr<Stream>> outputs;
    int inputAmount;
    int outputAmount;

public:
    virtual ~Device() = default;

    void addInput(shared_ptr<Stream> s){
        if (inputs.size() >= inputAmount) throw "INPUT STREAM LIMIT!";
        inputs.push_back(s);
        s->setTargetDevice(shared_from_this());
    }

    void addOutput(shared_ptr<Stream> s){
        if (outputs.size() >= outputAmount) throw "OUTPUT STREAM LIMIT!";
        outputs.push_back(s);
        s->setSourceDevice(shared_from_this());
    }

    virtual void updateOutputs() = 0;
};

/**
 * @class Mixer
 * @brief Смешивающее устройство
 */
class Mixer : public Device
{
private:
    int _inputs_count;

public:
    Mixer(int count){
        _inputs_count = count;
        inputAmount   = count;
        outputAmount  = 1;
    }

    void addInput(shared_ptr<Stream> s){
        if (inputs.size() >= _inputs_count)
            throw "Too much inputs";
        inputs.push_back(s);
        s->setTargetDevice(shared_from_this());
    }

    void addOutput(shared_ptr<Stream> s){
        if (outputs.size() >= MIXER_OUTPUTS)
            throw "Too much outputs";
        outputs.push_back(s);
        s->setSourceDevice(shared_from_this());
    }

    void updateOutputs() override {
        if (outputs.empty())
            throw "Should set outputs before update";

        double sum = 0;
        for (auto& st : inputs)
            sum += st->getMassFlow();

        outputs[0]->setMassFlow(sum);
    }
};

// Тесты

void test_DevicePointers(){
    cout << "TEST 1 (weak_ptr devices)... ";

    auto dev = make_shared<Mixer>(2);
    auto s = make_shared<Stream>(1);

    dev->addOutput(s);

    if (s->getSourceDevice() != nullptr)
        cout << "PASSED\n";
    else
        cout << "FAILED\n";
}

void test_WeakPtrExpired(){
    cout << "TEST 2 (expired)... ";

    weak_ptr<Device> weak;

    {
        auto dev = make_shared<Mixer>(1);
        auto s = make_shared<Stream>(1);
        dev->addOutput(s);
        weak = s->getSourceDevice();
    }

    if (weak.expired())
        cout << "PASSED\n";
    else
        cout << "FAILED\n";
}

void test_Mixer(){
    cout << "TEST 3 (Mixer update)... ";

    auto m = make_shared<Mixer>(2);
    auto s1 = make_shared<Stream>(1);
    auto s2 = make_shared<Stream>(2);
    auto out = make_shared<Stream>(3);

    s1->setMassFlow(10);
    s2->setMassFlow(5);

    m->addInput(s1);
    m->addInput(s2);
    m->addOutput(out);

    m->updateOutputs();

    if (fabs(out->getMassFlow() - 15) < POSSIBLE_ERROR)
        cout << "PASSED\n";
    else
        cout << "FAILED\n";
}

void test_TooManyInputs(){
    cout << "TEST 4 (too many inputs)... ";

    auto m = make_shared<Mixer>(1);
    auto s1 = make_shared<Stream>(1);
    auto s2 = make_shared<Stream>(2);

    m->addInput(s1);

    try {
        m->addInput(s2);
        cout << "FAILED\n";
    } catch (...) {
        cout << "PASSED\n";
    }
}

void test_NoOutput(){
    cout << "TEST 5 (no outputs)... ";

    auto m = make_shared<Mixer>(2);
    auto s1 = make_shared<Stream>(1);
    auto s2 = make_shared<Stream>(2);

    m->addInput(s1);
    m->addInput(s2);

    try {
        m->updateOutputs();
        cout << "FAILED\n";
    } catch (...) {
        cout << "PASSED\n";
    }
}

int main(){
    test_DevicePointers();
    test_WeakPtrExpired();
    test_Mixer();
    test_TooManyInputs();
    test_NoOutput();

    cout << "\n=== Демонстрация работы свойств source/target ===\n";

    auto A = make_shared<Mixer>(1);
    auto B = make_shared<Mixer>(1);
    auto s = make_shared<Stream>(100);

    s->setName("DemoStream");
    s->setMassFlow(12.5);

    // A → s → B
    A->addOutput(s);
    B->addInput(s);

    cout << "Поток: " << s->getName() << endl;

    auto src = s->getSourceDevice();
    auto trg = s->getTargetDevice();

    cout << "→ Вытекает из аппарата: " 
         << (src ? "Mixer A" : "Не задано") << endl;

    cout << "→ Втекает в аппарат:   " 
         << (trg ? "Mixer B" : "Не задано") << endl;

    return 0;
}

