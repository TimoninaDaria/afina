#include "SimpleLRU.h"
#include "iostream"
namespace Afina {
namespace Backend {

// See MapBasedGlobalLockImpl.h

void SimpleLRU::_cleaning(std::size_t delta){
    
    while(_current_size + delta  > _max_size){
        _current_size -= (_lru_head->key.length() + _lru_head->value.length());
        lru_node* new_head = _lru_head->next.get();
        new_head->prev = _lru_head->prev;
        _lru_head->next.release();
        _lru_index.erase(_lru_head->key);
        _lru_head.reset(new_head);
    }
	
    return;
} 

void SimpleLRU::_to_tail(lru_node* node){
    
    if (node->key == _lru_head->prev->key){ 
        return;
    }
  
    lru_node* next = node->next.get();
    lru_node* prev = node->prev;    
   
    if (node->key != _lru_head->key){
        node->next->prev = prev;
	prev->next.release();
	prev->next.reset(next);
	_lru_head->prev->next.reset(node);
        node->prev = _lru_head->prev;
	_lru_head->prev = node;
	node->next.release();
	
    }
    else{
	prev->next.reset(node);
        node->next.release();
	_lru_head.release();
        _lru_head.reset(next);	
    } 
}

bool SimpleLRU::Put(const std::string &key, const std::string &value) { 
    
    if ((key+value).length() > _max_size) {
        return false;
    }
         
    auto i = _lru_index.find(key);

    if (i == _lru_index.end()){
        _add_node(key,value);
    }
        
    else {
        _set_value(i->second.get(), value);
    }
    
    return true;
}
    
void SimpleLRU::_add_node(const std::string &key, const std::string &value){
    
    std::size_t delta = (key+value).length();
    _cleaning(delta);
    
    if(_lru_head){
        lru_node* tail = _lru_head->prev;
        tail->next.reset(new lru_node(key,value));
        tail->next->prev = tail;
        _lru_head->prev = tail->next.get();
        _lru_index.emplace(_lru_head->prev->key, *(tail->next));
        _current_size = _current_size + (key+value).length();
    }

    else {
        _lru_head.reset(new lru_node(key,value));
        _lru_head->prev = _lru_head.get();
        _lru_index.emplace(_lru_head->key, *_lru_head);
        _current_size = (key + value).length();
    }
}

// See MapBasedGlobalLockImpl.h
bool SimpleLRU::PutIfAbsent(const std::string &key, const std::string &value) {
       if ((key+value).length() > _max_size) {
        return false;
    }
        
    auto i = _lru_index.find(key);

    if (i == _lru_index.end()) {
        _add_node(key, value);
	return true;
    }
       
    return false;
}
           
void SimpleLRU::_set_value(lru_node& node, const std::string &value){
    
    _to_tail(&node);
    std::size_t delta =  value.length() - node.value.length();
    _cleaning(delta);
    
    _current_size = _current_size + value.length() - node.value.length();
    auto& old_value = node.value;
    old_value = value;
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
        _set_value(i->second.get(), value);	
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
    _to_tail(&(i->second.get()));

    return true;
}

} // namespace Backend
} // namespace Afina
