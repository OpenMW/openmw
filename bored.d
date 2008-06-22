/*
  OpenMW - The completely unofficial reimplementation of Morrowind
  Copyright (C) 2008  Nicolay Korslund
  Email: < korslund@gmail.com >
  WWW: http://openmw.snaptoad.com/

  This file (bored.d) is part of the OpenMW package.

  OpenMW is distributed as free software: you can redistribute it
  and/or modify it under the terms of the GNU General Public License
  version 3, as published by the Free Software Foundation.

  This program is distributed in the hope that it will be useful, but
  WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  General Public License for more details.

  You should have received a copy of the GNU General Public License
  version 3 along with this program. If not, see
  http://www.gnu.org/licenses/ .

 */

module bored;

import std.stdio;
import std.cstream;
import std.stream;
import std.random;
import std.file;

double rnd()
{
  return cast(double)std.random.rand()/uint.max;
}

int rand(int a, int b)
{
  return cast(int)((1+b-a)*rnd()+a);
}

void main()
{
  int gold = 0;
  int loot = 0;
  int level = 1;
  int life = 20;
  bool play = true;

  writefln("\nWelcome to Norrowind!");

  while(play)
    {
      writefln();
      if(life < 4) writefln("You are badly hurt.");
      else if(life < 10) writefln("You are injured.");
      else if(life < 15) writefln("You are slightly wounded.");
      if(gold) writefln("You have %d gold.", gold);
      if(loot) writefln("You have loot.");
      if(level>1) writefln("You are level ", level);
      writefln("
  1) Kill monster
  2) Read a book
  3) Read an NPC
  4) Sell some loot
  5) Walk around and get drunk on skooma
  6) Watch TEH k33wL P1XAL SHADID W4T3R!!!!1
  7) Exit
");
      uint input;
      dout.writef("Your choice: ");
      do
	{
	  input = cast(uint)(din.getc() - '0');
	}
      while(input >= 8);

      if(rnd() < 0.01)
	{
	  writefln("Program has performed an illegal instruction, the programmer will be shot.");
	  break;
	}

      writefln();

      if(((input == 5) || (input == 3) || (input == 1)) && (loot > 100))
	{
	  writefln("You are encumbered and cannot move. Try selling some of your junk.");
	  continue;
	}

      char[] str;
      int lootinc, goldinc;
      int oldlife = life;

      switch(input)
	{
	case 1: // Hunting
	  if(rnd() < 0.02)
	    {
	      writefln("You killed a Bodak and a Greater Mumm.. oops, wrong game, never mind.");
	      break;
	    }
	  if(rnd() < 0.02)
	    {
	      writefln(
"You were killed by an white striped aquatic flying dire gigant dragon
bear in a Balmora mansion. This is largely your own fault for using all
those plugins.");
	      play=false;
	      break;
	    }
	  switch(rand(0,15))
	    {
	    case 0: str = "Fjol the Outlaw"; goldinc = rand(0,70); lootinc = rand(10,120); break;
	    case 1: str = "a Betty Netch"; lootinc = rand(0,7); break;
	    case 2: str = "a Vampire"; goldinc = rand(0,10); lootinc = rand(20,40); break;
	    case 3: str = "a Dremora"; lootinc = rand(50,200); break;
	    case 4: str = "some NPC"; goldinc = rand(0,80); lootinc = rand(3,35); break;
	    case 5: str = "an Ordinator"; lootinc = rand(30,45); break;
	    case 6: str = "a Skeleton"; lootinc = 1; break;
	    case 7: str = "Fargoth"; goldinc = 10; lootinc = 4; break;
	    case 8: str = "a Cliff Racer"; lootinc = 2; break;
	    case 9: str = "Vivec"; lootinc = rand(0,20); goldinc = rand(0,60); life-=rand(1,2); break;
	    case 10: str = "a soultrapped Vivec"; goldinc = rand(0,60); lootinc = rand(100,300);
	      life-=rand(1,3); break;
	    case 11: str = "an Ascended Sleeper"; lootinc = rand(5,12); goldinc = rand(0,10); break;
	    case 12: str = "the entire town of Gnaar Mok"; goldinc = rand(40,50); lootinc = rand(70,140);
	      life-=rand(0,2); break;
	    case 13: str = "a Bethesda programmer for being so late with Oblivion"; break;
	    case 14: str = "a Werewolf. Which is kinda strange since you don't have Bloodmoon"; lootinc = rand(4,50); break;
	    case 15: str = "an important quest character. Way to go"; goldinc = rand(0,40); lootinc = rand(0,70); break;
	    }
	  if(rnd() < 0.65)
	    life -= rand(1,8);
	  if(life > 0)
	    {
	      writefln("You killed ", str, ".");
	      if(life < oldlife) writefln("You were hurt in the fight.");
	      else writefln("You survived the fight unscathed.");
	      if(goldinc) writefln("You got ", goldinc, " bucks.");
	      if(lootinc) writefln("You found some loot.");
	      gold += goldinc;
	      loot += lootinc;
	      if(rnd() < 0.2)
		{
		  writefln("You have gained a level!");
		  life += rand(3,10);
		  level++;
		}
	    }
	  else
	    {
	      writefln("You met ", str, " and were killed.");
	      play = false;
	    }
	  break;

	case 2:// Book
	  switch(rand(0,5))
	    {
	    case 0:
	      writefln("You read The History of The Emipire and fell asleep.");
	      break;
	    case 1:
	      writefln("You read The Pilgrim's Path and became a fanatical religious nut.");
	      break;
	    case 2:
	      writefln("You read the scroll 'Divine Intervention' and suddenly found yourself
outside, wearing only your night gown and slippers.");
	      break;
	    case 3:
	      writefln("You read Divine Metaphysics. Again");
	      if(rnd()<0.09)
		{
		  writefln("You discovered where the dwarwes went! And, more importantly, where
they stashed all their loot.");
		  loot += 1000;
		}
	      break;
	    case 4:
	      writefln("You learned a new skill.");
	      if(rnd() < 0.4) level++;
	      break;
	    case 5:
	      writefln("You dropped a book on you toe.");
	      life--;
	      if(life == 0)
		{
		  writefln("You are dead.");
		  play = false;
		}
	      break;
	    }
	  break;
	case 3://NPC
	  if(rnd()<0.05)
	    {
	      writefln("Nobody wants to speak with you.");
	      break;
	    }
	  writefln("You met an NPC");
	  switch(rand(0,9))
	    {
	    case 0: writefln("He had nothing interesting to say."); break;
	    case 1: writefln("She was really boring."); break;
	    case 2: writefln("You got a quest!"); break;
	    case 3: writefln("You completed a quest and got some dough."); gold += rand(1,10); break;
	    case 4: writefln("The nice NPC gave you a healing potion."); life+=rand(2,4); break;
	    case 5: writefln("You robbed 'em blind and got some loot."); loot+=(10,20); break;
	    case 6: writefln("The guard took some of your money, saying you were
late on your child support payments."); gold = gold/3; break;
	    case 7: writefln("You spent some money on bribes"); gold -= gold/4; break;
	    case 8: writefln("You had to travel all the way accross the island to talk to this person."); gold -= gold/4; break;
	    case 9: writefln("The Breton mistook you for his mother, and gave you tons of gold."); gold += 100; break;
	    }
	  break;

	case 4://Sell
	  if(loot == 0)
	    writefln("You have nothing to sell (except that moon sugar and the home made poetry that nobody wants)");
	  else if(rnd()<0.93)
	    {
	      goldinc = cast(int)(loot*rnd()*2);
	      if(goldinc > loot) writefln("The merchant likes you, you got ", goldinc, " gold for stuff worth only ", loot, ".");
	      if(goldinc <= loot) writefln("The merchant didn't like you, your ", loot, " worth of stuff
only got you ", goldinc, " gold.");
	    }
	  else
	    {
	      writefln("You met a talking mudcrab and an unfunny scamp! You got lots of\ncash for your loot.");
	      goldinc = 5*loot;
	    }
	  gold += goldinc;
	  loot = 0;
	  break;
	case 5://Skooma
	  switch(rand(0,7))
	    {
	    case 0:
	      str = "gigant, flesh eating mushrooms"; break;
	    case 1:
	      str = "a firm, slender and agile female argonian"; break;
	    case 2:
	      str = "dead people and some stupid guy in a golden mask"; break;
	    case 3:
	      str = "the whole world only being part of a computer game"; break;
	    case 4:
	      str = "nothing in particular"; break;
	    case 5:
	      str = "an old, half naked guy giving you orders, insisting you\ncall him 'spymaster'";
	      break;
	    case 6:
	      str = "being a geek who sits in front of a screen all day long"; break;
	    case 7:
	      str = "the clouds, man, the crazy clouds!"; break;
	    }
	  writefln("You fall asleep in a ditch and dream about ", str, ".");
	  break;
	case 6: //Water effects
	  switch(rand(0,5))
	    {
	    case 0: writefln("Them waves sure are pretty!"); break;
	    case 1:
	      writefln("A slaughter fish jumps up and bites you in the nose.");
	      life--;
	      if(life == 0)
		{
		  writefln("You are dead.");
		  play = false;
		}
	      break;
	    case 2: writefln("Those graphics might have looked impressive six years ago...");
	      break;
	    case 3: writefln("You were eaten by a Mudcrab. You are dead."); play=false; break;
	    case 4: writefln("You suddenly realize that the person who made this program has way too much time on his hands.");break;
	    case 5: writefln("You found a note with cheat codes on them."); level+=2; life+=rand(5,15); break;
	    }
	  break;

	// Exit
	case 7: play=false; break;
	}
    }
  writefln("\nScore:");
  writefln("Gold: %d : %d points", gold, gold);
  writefln("Level: %d : %d points", level, (level-1)*40);
  if(loot) writefln("Loot: you have to sell the loot to get any points for it.");
  Entry n;

  n.score = gold + (level-1) * 40;

  writefln("Total score: ", n.score);

  Entry[] high = getScores();




  int index = 10;

  foreach(int i, Entry e; high)
    if(n.score > e.score)
      {
	index = i;
	break;
      }

  writefln();

  if(index != 10)
    {
      writef("Congratulations! You've made it to the Hall of Fame.\nEnter your name: ");
      din.readLine();
      n.name = din.readLine();

      for(int i = 9; i>index; i--)
	high[i] = high[i-1];

      high[index] = n;

      setScores(high);
    }

  writefln("Hall of Fame:");
  foreach(int i, Entry e; high)
    if(e.score) writefln("%-2d: %-10d   %s", i+1, e.score, e.name);

  writefln("\n(Apologies to Bethesda Softworks)");
}

struct Entry
{
  char[] name;
  int score;
}

void setScores(Entry[] l)
{
  auto File f = new File("bored.highscores", FileMode.OutNew);
  foreach(Entry e; l)
    {
      f.write(e.name);
      f.write(e.score);
    }
}

Entry[] getScores()
{
  Entry[] l;
  l.length = 10;

  if(exists("bored.highscores"))
    {
      auto File f = new File("bored.highscores");
      foreach(ref Entry e; l)
	{
	  f.read(e.name);
	  f.read(e.score);
	}
    }

  return l;
}
