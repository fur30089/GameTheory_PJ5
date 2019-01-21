#pragma once
#include <string>
#include <random>
#include <sstream>
#include <map>
#include <type_traits>
#include <algorithm>
#include "board.h"
#include "action.h"
#include "weight.h"
#include <fstream>

float lr=0.1/40;
float explorationRate=0;

std::vector<weight> net;
	void init_weights(const std::string& info) {    
  for(int i=0;i<32;i++){
  net.emplace_back(pow(15,6));} 		
} 

class agent {
public:
	agent(const std::string& args = "") {
		std::stringstream ss("name=unknown role=unknown " + args);
		for (std::string pair; ss >> pair; ) {
			std::string key = pair.substr(0, pair.find('='));
			std::string value = pair.substr(pair.find('=') + 1);
			meta[key] = { value };
		}
	}
	virtual ~agent() {}
	virtual void open_episode(const std::string& flag = "") {}
	virtual void close_episode(const std::string& flag = "") {}
	virtual action take_action(const board& b, int)  {  return action(); } 
 	virtual bool check_for_win(const board& b) { return false; }

public:
	virtual std::string property(const std::string& key) const { return meta.at(key); }
	virtual void notify(const std::string& msg) { meta[msg.substr(0, msg.find('='))] = { msg.substr(msg.find('=') + 1) }; }
	virtual std::string name() const { return property("name"); }
	virtual std::string role() const { return property("role"); }

	virtual void load_weights(const std::string& path) {
		std::ifstream in(path, std::ios::in | std::ios::binary);
		if (!in.is_open()) std::exit(-1);
		uint32_t size;
		in.read(reinterpret_cast<char*>(&size), sizeof(size));
		net.resize(size);
		for (weight& w : net) in >> w;
		in.close();
	}
	virtual void save_weights(const std::string& path) {
		std::ofstream out(path, std::ios::out | std::ios::binary | std::ios::trunc);
		if (!out.is_open()) std::exit(-1);
		uint32_t size = net.size();
		out.write(reinterpret_cast<char*>(&size), sizeof(size));
		for (weight& w : net) out << w;
		out.close();
	}  
  //virtual int hint() { return 0; }

protected:
	typedef std::string key;
	struct value {
		std::string value;
		operator std::string() const { return value; }
		template<typename numeric, typename = typename std::enable_if<std::is_arithmetic<numeric>::value, numeric>::type>
		operator numeric() const { return numeric(std::stod(value)); }
	};
	std::map<key, value> meta;
};

class random_agent : public agent {
public:
	random_agent(const std::string& args = "") : agent(args) {
		if (meta.find("seed") != meta.end())
			engine.seed(int(meta["seed"]));
	}
	virtual ~random_agent() {}

protected:
	std::default_random_engine engine;
};

/**
 * base agent for agents with weight tables
 */
class weight_agent : public agent {  //public inheritance
//class player : public agent {  //public inheritance
public:
	weight_agent(const std::string& args = "") : agent("role=player "+args) {  //constructor
		if (meta.find("init") != meta.end()) // pass init=... to initialize the weight
			init_weights(meta["init"]);
		if (meta.find("load") != meta.end()) // pass load=... to load from a specific file
			load_weights(meta["load"]);
	}
	virtual ~weight_agent() {  // ~ desconstructor??
		if (meta.find("save") != meta.end()) // pass save=... to save to a specific file
			save_weights(meta["save"]);
	}

	virtual void open_episode(const std::string& flag = "") {}
	/*
  virtual void close_episode(const std::string& flag = "") {  // need to train the value of state when game overs
		// ...

		rec.clear();
	}
 */
 
 /*
	virtual action take_action(const board& b) {  // record state reward to rec
	
		for (int op = 0; op < 4; op++) {
			action a = action::slide(op);
			board after = b;
			int reward = a.apply(after);
			//rec.push_back({ after, reward });
		}

		return a;
	}
 */
 	virtual action take_action(const board& before, int lastmove) {  // record state reward to rec
      //TableB[1][1][1]=100;
      //std::cout<< TableB[1][1][1];
      //std::cout<< before;
      //std::cout<<net[0][20]<<"\n";
      //std::cout<<"player"<<net[4][20]<<"\n"; 
      board::reward reward;
      board afterbest;
      int a=10,r; 
      float value,score=-10000000;  // if score < score, some error happens
      float y=1;
      for(int i=0;i<=3;i++){   //select a move
        board after = before;
        reward = after.slide(i);
        //std::cout<< after; 
        if (reward != -1){     
          if (reward+y*estimate(after)>=score){
          r=reward;
          value=estimate(after); 
          a=i; 
          score=r+y*value; 
          afterbest=after;      
          } 
        }
      } 

      //std::cout<<a<<'\n';
      //std::cout<<"reward:"<<r<<'\n'; 
      //std::cout<<"value:"<<value<<'\n';

			if (a != 10){   
      //std::cout<<"bag: "<<lastmove<<'\n';           //training
      
      float offset=lr*(r+y*value-estimate(last));
      update(last,offset);
      last=afterbest;

      return action::slide(a); 
      } 
      //return action::slide(0); 
      else 
      {   //std::cout<<"a: "<<a<<'\n';
          //float offset=lr*(r+y*value-estimate(last));
          update(last,-50);         
          return action();  // 0 1 2 3  ileagal??
      } 
             
	}
 
	virtual bool check_for_win(const board& b) { return false; }

	float estimate(board b) {                // sum of vlue of every tuple.  map tiles to index of tuple, and get the value from the tuple index. 
		float v = 0;
    //std::cout<<"hahahahahahha";
    for(int i=0;i<8;i++){
       //std::cout<<b;
      v+=net[i*4][b(0)*pow(15,5)+b(4)*pow(15,4)+b(8)*pow(15,3)+b(12)*pow(15,2)+b(9)*15+b(13)];
      v+=net[i*4+1][b(1)*pow(15,5)+b(5)*pow(15,4)+b(9)*pow(15,3)+b(13)*pow(15,2)+b(10)*15+b(14)];
      v+=net[i*4+2][b(1)*pow(15,5)+b(5)*pow(15,4)+b(9)*pow(15,3)+b(2)*pow(15,2)+b(6)*15+b(10)];
      v+=net[i*4+3][b(2)*pow(15,5)+b(6)*pow(15,4)+b(10)*pow(15,3)+b(3)*pow(15,2)+b(7)*15+b(11)];
      b=rotate_right(b);
      if (i==3){
        b=reflect_horizontal(b);
      }
    }	    
    return v;
	}

	void update(board b, float offset) {   // update the value

    for(int i=0;i<8;i++){
      net[i*4][b(0)*pow(15,5)+b(4)*pow(15,4)+b(8)*pow(15,3)+b(12)*pow(15,2)+b(9)*15+b(13)]+= offset / 32;
      net[i*4+1][b(1)*pow(15,5)+b(5)*pow(15,4)+b(9)*pow(15,3)+b(13)*pow(15,2)+b(10)*15+b(14)]+= offset / 32;
      net[i*4+2][b(1)*pow(15,5)+b(5)*pow(15,4)+b(9)*pow(15,3)+b(2)*pow(15,2)+b(6)*15+b(10)]+= offset / 32;
      net[i*4+3][b(2)*pow(15,5)+b(6)*pow(15,4)+b(10)*pow(15,3)+b(3)*pow(15,2)+b(7)*15+b(11)]+= offset / 32;
      b=rotate_right(b);
      if (i==3){
        b=reflect_horizontal(b);
      }
    }	    
    
  }


	board reflect_horizontal(board state) {
		for (int r = 0; r < 4; r++) {
			std::swap(state[r][0], state[r][3]);
			std::swap(state[r][1], state[r][2]);
		}
	  return state; 
   }

	board transpose(board state) {
		for (int r = 0; r < 4; r++) {
			for (int c = r + 1; c < 4; c++) {
				std::swap(state[r][c], state[c][r]);
			}
		}
	  return state; 	
  }

	board rotate_right(board state) { 
  state=transpose(state); 
  state=reflect_horizontal(state);
  return state;
   } 
	

  


protected:
/*
	virtual void init_weights(const std::string& info) {
    for(int i=0;i<32;i++){
    net.emplace_back(pow(15,6));} 
	}
	virtual void load_weights(const std::string& path) {
		std::ifstream in(path, std::ios::in | std::ios::binary);
		if (!in.is_open()) std::exit(-1);
		uint32_t size;
		in.read(reinterpret_cast<char*>(&size), sizeof(size));
		net.resize(size);
		for (weight& w : net) in >> w;
		in.close();
	}
	virtual void save_weights(const std::string& path) {
		std::ofstream out(path, std::ios::out | std::ios::binary | std::ios::trunc);
		if (!out.is_open()) std::exit(-1);
		uint32_t size = net.size();
		out.write(reinterpret_cast<char*>(&size), sizeof(size));
		for (weight& w : net) out << w;
		out.close();
	}
*/
protected:
	//std::vector<weight> net;

//	struct record {
//		board after;
//		int reward;
//	};
//	std::vector<record> rec;
	board last;
};


/**
 * base agent for agents with a learning rate
 */
class learning_agent : public agent {
public:
	learning_agent(const std::string& args = "") : agent(args), alpha(0.1f) {
		if (meta.find("alpha") != meta.end())
			alpha = float(meta["alpha"]);
	}
	virtual ~learning_agent() {}

protected:
	float alpha;
};





int hint_tile;
int step;
float N_bonustile;
float N_totaltile;
class rndenv : public random_agent {
public:
  
	rndenv(const std::string& args = "") : random_agent("name=random role=environment " + args),
		space({ 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15 }), space2({0,1,2}),space3({std::array<int, 4>{12,13,14,15},std::array<int, 4>{0,4,8,12},std::array<int, 4>{0,1,2,3},std::array<int, 4>{3,7,11,15}}) ,
    baga({1,1,1,1,2,2,2,2,3,3,3,3}),IFBonus(1,21),exploration(1,10)
    {		
    if (meta.find("init") != meta.end()) // pass init=... to initialize the weight
			init_weights(meta["init"]);
		if (meta.find("load") != meta.end()) // pass load=... to load from a specific file
			load_weights(meta["load"]);} 
	virtual ~rndenv() {  // ~ desconstructor??
		if (meta.find("save") != meta.end()) // pass save=... to save to a specific file
			save_weights(meta["save"]);
	}

	virtual action take_action(const board& after,int lastmove) { 
    //hint_tile=2;
    //net[30][20]=1;
    //std::cout<<"evil"<<net[4][20]<<"\n";
    int tile;
    int sum=0;
    for (int i=0;i<=15;i++){
      sum+=after(i);
    }  
    
    if (sum==0 ) {
      step=0;
      N_bonustile=0;
      N_totaltile=0;      
      baga={1,1,1,1,2,2,2,2,3,3,3,3};
      std::shuffle(baga.begin(), baga.end(), engine);
      tile = baga.back();
 		  baga.pop_back();
      hint_tile= baga.back();
      //std::cout<<"bagoriginal"<<"\n"; 
      //baga.pop_back();            
		}
    else{tile=hint_tile;}
      
    if (baga.empty()) {
      baga={1,1,1,1,2,2,2,2,3,3,3,3};
      std::shuffle(baga.begin(), baga.end(), engine);
      //std::cout<<"bagoriginal"<<"\n";      
		}

    int Vmax=0;
     
    for(int i=0;i<=15;i++){
        if (after(i)>Vmax){
          Vmax=after(i);
        }
    }
   
    
    //if (Vmax>=7 & IFBonus(engine)==1)
    if (Vmax>=7 & (((N_bonustile+1)/(N_totaltile+1)) <= 0.047))
    //if (Vmax>=7)
       { 
             std::uniform_int_distribution<int> bonustile(4,4+(Vmax-7));
             hint_tile=bonustile(engine);
             N_bonustile++;
             N_totaltile++;
             //hint_tile=4;
             //baga.pop_back();
             //tile=5;
             //std::cout<<"bonustile:"<<tile<<"\n";                       
       }
    else{
		hint_tile = baga.back();
    N_totaltile++;
 		baga.pop_back();
    }
   
		//hint_tile = baga.back();
 		//baga.pop_back();   
    
    //std::cout<<"tile:"<<tile<<"\n";
    

/* 
    step++;   
    //std::cout<<"lastmove:"<<lastmove<<"\n";   
    if (step<=9){ 
          std::shuffle(space.begin(), space.end(), engine);
      		for (int pos : space) {
      			if (after(pos) != 0) continue;
      			return action::place(pos, tile);
      		}
        }
    else{ 
          std::shuffle(space3[lastmove].begin(), space3[lastmove].end(), engine);
      		for (int pos : space3[lastmove]) {
      			if (after(pos) != 0) continue;
      			return action::place(pos, tile);
      		}
        } 

 
*/
    step++;   
    //std::cout<<"lastmove:"<<lastmove<<"\n";   
    if (step<=9){ 
          std::shuffle(space.begin(), space.end(), engine);
      		for (int pos : space) {
      			if (after(pos) != 0) continue;
      			return action::place(pos, tile);
      		}
        }
    else{ 
           if(exploration(engine)<=explorationRate*10)
          {
            std::shuffle(space3[lastmove].begin(), space3[lastmove].end(), engine);
        		for (int pos : space3[lastmove]) {
        			if (after(pos) != 0) continue;
        			return action::place(pos, tile);
        		}
          }  
          
          else
          { 
          float score2=10000000;  // if score < score, some error happens
          float lr=0.1/8,y=1; 
          int bestpos;         
      		for (int pos : space3[lastmove]) {
              board before = after;
        			if (before(pos) != 0) continue;
              before(pos)=tile;
              
              board::reward reward;
              int a=10; 
              float value,score=-10000000;  // if score < score, some error happens
              for(int i=0;i<=3;i++){   //select a move
                board after = before;
                reward = after.slide(i);
                //std::cout<< after; 
                if (reward != -1){     
                  if (reward+y*estimate(after)>=score){
                  score=reward+y*estimate(after); 
                  a=i;      
                  } 
                }
              }               
              if (a==10){
               bestpos=pos;
               break;
              }
              
              if (score<=score2){
              score2=score;
              bestpos=pos;      
              }                
      		};                           
      			return action::place(bestpos, tile);        
         }         
       }
		return action();
}


	float estimate(board b) {                // sum of vlue of every tuple.  map tiles to index of tuple, and get the value from the tuple index. 
		float v = 0;
    //std::cout<<"hahahahahahha";
    for(int i=0;i<8;i++){
       //std::cout<<b;
      v+=net[i*4][b(0)*pow(15,5)+b(4)*pow(15,4)+b(8)*pow(15,3)+b(12)*pow(15,2)+b(9)*15+b(13)];
      v+=net[i*4+1][b(1)*pow(15,5)+b(5)*pow(15,4)+b(9)*pow(15,3)+b(13)*pow(15,2)+b(10)*15+b(14)];
      v+=net[i*4+2][b(1)*pow(15,5)+b(5)*pow(15,4)+b(9)*pow(15,3)+b(2)*pow(15,2)+b(6)*15+b(10)];
      v+=net[i*4+3][b(2)*pow(15,5)+b(6)*pow(15,4)+b(10)*pow(15,3)+b(3)*pow(15,2)+b(7)*15+b(11)];
      b=rotate_right(b);
      if (i==3){
        b=reflect_horizontal(b);
      }
    }	    
    return v;
	}

	board reflect_horizontal(board state) {
		for (int r = 0; r < 4; r++) {
			std::swap(state[r][0], state[r][3]);
			std::swap(state[r][1], state[r][2]);
		}
	  return state; 
   }

	board transpose(board state) {
		for (int r = 0; r < 4; r++) {
			for (int c = r + 1; c < 4; c++) {
				std::swap(state[r][c], state[c][r]);
			}
		}
	  return state; 	
  }

	board rotate_right(board state) { 
  state=transpose(state); 
  state=reflect_horizontal(state);
  return state;
   } 


int hint() { 
if (hint_tile>3)
{return 4;}
else
{return hint_tile;}
} 

	
 
private:
  
	std::array<int, 16> space;   //declare space here??
  std::array<int, 3> space2;
  //std::array<int, 21> IFBonus;
  //char space3[4][4];
  std::array<std::array<int, 4>,4> space3;
	std::uniform_int_distribution<int> IFBonus;
  std::uniform_int_distribution<int> exploration;
  std::vector<int> baga;  
};



/**
 * dummy player
 * select a legal action randomly
 */
 
 
class player : public random_agent {
public:
	player(const std::string& args = "") : random_agent("name=dummy role=player " + args),
		opcode({ 0, 1, 2, 3 }) {}

	virtual action take_action(const board& before,int hint) {
		std::shuffle(opcode.begin(), opcode.end(), engine);
		for (int op : opcode) {
			board::reward reward = board(before).slide(op);
			if (reward != -1) return action::slide(op);
		}
		return action();
	}

private:
	std::array<int, 4> opcode;
};

