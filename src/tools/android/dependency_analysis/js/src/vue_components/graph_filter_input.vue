<!-- Copyright 2020 The Chromium Authors. All rights reserved.
     Use of this source code is governed by a BSD-style license that can be
     found in the LICENSE file. -->

<template>
  <div>
    <label
        class="md-subheading"
        for="filter-input">
      Add Node
    </label>
    <Autocomplete
        id="filter-input"
        ref="autocomplete"
        :search="search"
        :get-result-value="getResultValue"
        @submit="onSelectOption"/>
  </div>
</template>

<script>
import {CUSTOM_EVENTS} from '../vue_custom_events.js';

import Autocomplete from '@trevoreyre/autocomplete-vue';

// @vue/component
const GraphFilterInput = {
  components: {
    Autocomplete,
  },
  props: {
    nodeIds: Array,
    nodesAlreadyInFilter: Array,
    getShortName: Function,
  },
  data: function() {
    return {
      // Sorts the nodes by their short names, which will be displayed.
      // this.getShortName() is cached to improve performance (~150 ms at load).
      nodeIdsSortedByShortNames: this.nodeIds
          .map(name => ({
            realName: name,
            shortName: this.getShortName(name),
          }))
          .sort((a, b) => a.shortName.localeCompare(b.shortName))
          .map(nameObj => nameObj.realName),
    };
  },
  computed: {
    nodesAlreadyInFilterSet: function() {
      return new Set(this.nodesAlreadyInFilter.map(
          filterEntry => filterEntry.name));
    },
  },
  methods: {
    getResultValue: function(result) {
      return this.getShortName(result);
    },

    search: function(searchTerm) {
      const searchTermLower = searchTerm.toLowerCase();
      return this.nodeIdsSortedByShortNames.filter(name => {
        return !this.nodesAlreadyInFilterSet.has(name) &&
          name.toLowerCase().includes(searchTermLower);
      });
    },

    onSelectOption(nodeNameToAdd) {
      if (!this.nodeIdsSortedByShortNames.includes(nodeNameToAdd)) {
        return;
      }
      this.$emit(CUSTOM_EVENTS.FILTER_SUBMITTED, nodeNameToAdd);
      this.$refs.autocomplete.value = '';
    },
  },
};

export default GraphFilterInput;
</script>

<style>
#filter-input {
  width: 100%;
}

.autocomplete-result-list {
  background: #fff;
  box-sizing: content-box;
  list-style: none;
  margin: 0;
  max-height: 40vh;
  overflow-y: auto;
  padding: 0;
  /* !important since Autocomplete hard-codes z-index into its HTML template */
  z-index: 10 !important;
}

.autocomplete-result {
  word-wrap: break-word;
}

.autocomplete-result:hover,
.autocomplete-result[aria-selected=true] {
  background-color: rgba(0, 0, 0, 0.1);
}
</style>
