npc_id = 999999;
npc_level = 0;
npc_hp  = 0;
npc_type = -1;
npc_x = 0;
npc_y = 0;

function set_npc( id, level, hp, type, x, y)
	npc_id = id;
	npc_level = level;
	npc_hp = hp;
	npc_type = type;
	npc_x = x;
	npc_y = y;
end

function event_player_move( player )
	player_x = API_get_x(player);
	player_y = API_get_y(player);
	my_x = API_get_x(npc_id);
	my_y = API_get_y(npc_id);

	if(player_x == my_x) then
		if(player_y == my_y) then
			
		end
	end
end
