// Included in baseline_bk_boundaries_test.cpp's anonymous namespace, after helper()/idle().
// Option-box sprite component (bk_menu_sprite_thunks.hpp): the actual guest menu owner scope with the box's
// reserved sprite ID and the actual billboard group helpers, in the thunk's order, at the live gfx cursor after
// E2E48's own MODELVIEW LOAD|PUSH. Without this owner the sprite's world transform kept RT64's default AUTO group
// (no group command at all), whose AUTO position test snaps a first step from rest; the emitted group must carry
// the sprite ID with position INTERPOLATE, open exactly once and close only for the opening activation.
unsigned sprite_group_cases(){
    constexpr uint32_t offset_word=bk+offsetof(TooieBkState,cur_model_transform_id_offset);
    constexpr uint32_t handler_depth=bk+offsetof(TooieBkState,handler_depth);
    constexpr uint32_t model_id=0x30000600u, sprite_id=model_id+0x100u;   // slot0, box0, role extra; sprite component
    constexpr uint32_t e2e48_sp=0x807FC000u;
    // Public gEXMatrixGroup word2 fields (baseline_feather_cut_cases.inc): push0 proj1 mode2 pos3 rot5 scale7 skew9
    // persp11 vert13 tile15 order17 edit19 aspect20 tc22 lookat24; COMPONENT SKIP0 INTERPOLATE1 AUTO2.
    constexpr uint32_t live=1u|(1u<<2)|(1u<<3)|(1u<<5)|(1u<<7)|(1u<<9)|(1u<<11)|(1u<<15)|(2u<<24);
    static_assert(live==0x02008AADu && ((live>>3)&3u)==1u,
        "bk_pipeline.c decomposed non-map live policy: position INTERPOLATE (the AUTO default is 2)");
    unsigned cases=0;
    for(uint32_t cut:{0u,1u}){
        uint8_t* rdram=fresh();
        const uint32_t token=helper(rdram,candidate::tooie_bk_menu_model_begin,sprite_id,cut);
        require(token&&word(rdram,f_kind)==TOOIE_BK_OWNER_MENU&&word(rdram,f_id)==sprite_id,
            "the reserved sprite component is accepted by the menu owner scope");
        put(rdram,offset_word,3);   // L14-VR01b: a live traversal offset the sprite group must not use
        helper(rdram,candidate::tooie_bk_handler_begin,buffers,0,e2e48_sp);
        require(word(rdram,buffers)==cursor0+16u&&word(rdram,cursor0)==0x6400000Cu&&word(rdram,cursor0+4)==sprite_id&&
            word(rdram,cursor0+8)==(cut?1u:live)&&!word(rdram,cursor0+12)&&word(rdram,handler_depth)==1u,
            cut?"fresh box: the sprite ID with every component SKIP, like its model"
               :"live box: the sprite's own MODELVIEW PUSH group, position INTERPOLATE");
        require(word(rdram,offset_word)==3u,"L14-VR01b: the guest restores the traversal offset after the sprite group");
        // The original E2180 draw and E2E48's own MODELVIEW pop run here.
        helper(rdram,candidate::tooie_bk_handler_end,buffers,e2e48_sp+8u);
        require(word(rdram,buffers)==cursor0+16u&&word(rdram,handler_depth)==1u,"a foreign activation never closes the group");
        helper(rdram,candidate::tooie_bk_handler_end,buffers,e2e48_sp);
        require(word(rdram,buffers)==cursor0+24u&&word(rdram,cursor0+16)==0x6400000Du&&word(rdram,cursor0+20)==1u&&
            !word(rdram,handler_depth),"exactly one MODELVIEW group pop for the opening activation");
        helper(rdram,candidate::tooie_bk_menu_model_end,token);
        idle(rdram,0,"sprite scope end restores the unowned context");
        ++cases;
    }
    // Current behaviour (no sprite owner): the same helper emits nothing, so RT64 keeps the AUTO default group.
    {
        uint8_t* rdram=fresh();
        helper(rdram,candidate::tooie_bk_handler_begin,buffers,0,e2e48_sp);
        require(word(rdram,buffers)==cursor0&&word(rdram,handler_depth)==1u,"no owner: no group command is emitted");
        helper(rdram,candidate::tooie_bk_handler_end,buffers,e2e48_sp);
        require(word(rdram,buffers)==cursor0&&!word(rdram,handler_depth),"an unopened entry closes without a pop");
        ++cases;
    }
    // L14-VR01b: a graph billboard (nonzero node) still takes its traversal ordinal, exactly as before.
    {
        uint8_t* rdram=fresh();
        const uint32_t token=helper(rdram,candidate::tooie_bk_menu_model_begin,sprite_id,0u);
        require(token!=0u,"owner scope for the billboard ordinal case");
        put(rdram,offset_word,3);
        helper(rdram,candidate::tooie_bk_handler_begin,buffers,0x80500000u,e2e48_sp);
        require(word(rdram,buffers)==cursor0+16u&&word(rdram,cursor0+4)==sprite_id+3u&&word(rdram,cursor0+8)==live&&
            word(rdram,offset_word)==3u,"a billboard group keeps its traversal ordinal and the offset");
        helper(rdram,candidate::tooie_bk_handler_end,buffers,e2e48_sp);
        require(word(rdram,buffers)==cursor0+24u&&!word(rdram,handler_depth),"the billboard group closes once");
        helper(rdram,candidate::tooie_bk_menu_model_end,token);
        idle(rdram,0,"billboard ordinal case restores the unowned context");
        ++cases;
    }
    return cases;
}
