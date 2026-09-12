static int main(){
    _gpu vector<int> a;
    _gpu vector<int> b;
    _gpu vector<int> c = AddLists(a,b);

    for(uint i = 0; i < a.size(); i++){
        int current = c[i];
        print(current);

        if(current == 69){
            print("nice");
        }
    }
}

static _gpu vector<int> AddLists(_gpu vector<int> a,_gpu vector<int> b){
    _gpu vector<int> c;
    AddNumbers(a.size(),a,b,c);
    return c;
}

static int Add(int a, int b){
    return a + b;
}

// id is the index, this is predefined
// the compiler will also add boudscheck
kernel void AddNumbers(vector<int> a, vector<int> b, vector<int> c){
    c[id] = Add(a[id], b[id])
}