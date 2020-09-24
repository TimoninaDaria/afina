#include "SimpleLRU.h"
#include "iostream"
namespace Afina {
namespace Backend {

// See MapBasedGlobalLockImpl.h
bool SimpleLRU::Put(const std::string &key, const std::string &value) { 
    
    if ((key+value).length() > _max_size) {
        return false;
    }
        
    if(_lru_head){
    
        auto i = _lru_index.find(key);

        if (i == _lru_index.end()) {
        
            while(_current_size + (key+value).length() > _max_size){
                _current_size -= (_lru_head->key.length() + _lru_head->value.length());
                lru_node* new_head = _lru_head->next.get();
                new_head->prev = _lru_head->prev;
                _lru_head->next.release();
                _lru_index.erase(_lru_head->key);
                _lru_head.reset(new_head);
            }
    
        
            lru_node* tail = _lru_head->prev;
            tail->next.reset(new lru_node(key,value));
            tail->next->prev = tail;
            _lru_head->prev = tail->next.get();
            _lru_index.emplace(_lru_head->prev->key, *(tail->next));
            _current_size = _current_size + (key+value).length();
        }
        
        else {
        
            while(_current_size + value.length() - i->second.get().value.length() > _max_size){
                _current_size -= (_lru_head->key.length() + _lru_head->value.length());
                lru_node* new_head = _lru_head->next.get();
                new_head->prev = _lru_head->prev;
                _lru_head->next.release();
                _lru_index.erase(_lru_head->key);
                _lru_head.reset(new_head);
        
            }
            _current_size = _current_size + value.length() - i->second.get().value.length();
            auto& old_value = i->second.get().value; 
            old_value = value;
        }
    }
    
    else {
        _lru_head.reset(new lru_node(key,value));
        _lru_head->prev = _lru_head.get();
        _lru_index.emplace(_lru_head->prev->key, *_lru_head->prev);
        _current_size = (key + value).length();
    }
    
    return true;
}
    
 
// See MapBasedGlobalLockImpl.h
bool SimpleLRU::PutIfAbsent(const std::string &key, const std::string &value) {
       if ((key+value).length() > _max_size) {
        return false;
    }
        
    if(_lru_head){
    
        auto i = _lru_index.find(key);

        if (i == _lru_index.end()) {
        
            while(_current_size + (key+value).length() > _max_size){
                _current_size -= (_lru_head->key.length() + _lru_head->value.length());
                lru_node* new_head = _lru_head->next.get();
                new_head->prev = _lru_head->prev;
                _lru_head->next.release();
                _lru_index.erase(_lru_head->key);
                _lru_head.reset(new_head);
            }
    
        
            lru_node* tail = _lru_head->prev;
            tail->next.reset(new lru_node(key,value));
            tail->next->prev = tail;
            _lru_head->prev = tail->next.get();
            _lru_index.emplace(_lru_head->prev->key, *(tail->next));
            _current_size = _current_size + (key+value).length();
        }
        
        else {
        
            return false;
        }
    }
    
    else {
        _lru_head.reset(new lru_node(key,value));
        _lru_head->prev = _lru_head.get();
        _lru_index.emplace(_lru_head->prev->key, *_lru_head->prev);
        _current_size = (key + value).length();
    }
    
    return true;
}
            
// See MapBasedGlobalLockImpl.h
bool SimpleLRU::Set(const std::string &key, const std::string &value){
     if ((key+value).length() > _max_size) {
         return false;
     }
        
    auto i = _lru_index.find(key);

    if (i == _lru_index.end()) {
        return false;
    }
        
    else {
        
        while(_current_size + value.length() - i->second.get().value.length() > _max_size){
            _current_size -= (_lru_head->key.length() + _lru_head->value.length());
            lru_node* new_head = _lru_head->next.get();
            new_head->prev = _lru_head->prev;
            _lru_head->next.release();
            _lru_index.erase(_lru_head->key);
            _lru_head.reset(new_head);
        
        }
        _current_size = _current_size + value.length() - i->second.get().value.length();
        auto& old_value = i->second.get().value; 
        old_value = value;
    }

    return true;
}



// See MapBasedGlobalLockImpl.h
bool SimpleLRU::Delete(const std::string &key) {
   
    auto i = _lru_index.find(key);

    if (i == _lru_index.end()) { 
        return false;
    }
    
    _current_size -=  (key.length() + i->second.get().value.length());
    auto& element = i->second.get();
    auto next_element = element.next.get();
    auto prev_element = element.prev;
    _lru_index.erase(key);
    
    if (next_element){

        next_element->prev = prev_element;
        
        if (key == _lru_head->key){

            _lru_head->next.release();
            _lru_head.reset(next_element);    

        }
        
        else{
            prev_element->next.reset(next_element); 
        }
    } 
    
    else {
    
        _lru_head->prev = prev_element; 
        if (key == _lru_head->key){
            _lru_head->next.release();
            _lru_head.reset(next_element);    
        }
        else{
            prev_element->next.reset(next_element); 
        }
    }
    
    return true;    
}

    
// See MapBasedGlobalLockImpl.h
bool SimpleLRU::Get(const std::string &key, std::string &value)
{
    auto i = _lru_index.find(key);
    if(i == _lru_index.end())
    {
       return false;
    }
    
    value = i->second.get().value;
    return true;
}

} // namespace Backend
} // namespace Afina
